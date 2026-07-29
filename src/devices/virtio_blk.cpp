/*
Copyright 2026 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "../../include/devices/virtio_blk.hpp"
#include "../../include/machine.hpp"

#include <cassert>
#include <cstdio>
#include <sys/stat.h>

VirtIO_BLK::VirtIO_BLK(uint64_t base, uint64_t size, Machine& cpu, fdt_node* fdt, FILE* image)
	: Device(base, size, fdt, cpu.get_mmap()), plic(cpu.get_mmio()->get<PLIC>().get()), irq_num(plic->acquire_irq()), disk(image)
{
	cpu.get_mmap()->add_region(base, size);

	fdt_node* virtio_blk_node = fdt_node_create_reg("virtio_mmio", base);
	fdt_node_add_prop(virtio_blk_node, "compatible", "virtio,mmio\0", 12);
	fdt_node_add_prop_reg(virtio_blk_node, "reg", base, size);
	fdt_node* soc  = fdt_node_find(fdt, "soc");
	fdt_node* plic = fdt_node_find_reg(soc, "plic", 0x0C000000);
	fdt_node_add_prop_u32(virtio_blk_node, "interrupt-parent", fdt_node_get_phandle(plic));
	fdt_node_free(plic);
	fdt_node_add_prop_u32(virtio_blk_node, "interrupts", irq_num);
	fdt_node_add_child(soc, virtio_blk_node);
	fdt_node_free(soc);

	// Device features
	device_features = VIRTIO_F_VERSION_1;

	FILE* uart_out = cpu.get_uart_output();
	if(!disk)
	{
		// can't open image -> device will return IO errors on ops
		fprintf(uart_out, "virtio_blk: image is invalid\n");
		fflush(uart_out);
	}
	else
	{
		// determine capacity
		fseek(disk, 0, SEEK_END);
		long sz = ftell(disk);
		if(sz < 0) sz = 0;
		capacity_sectors = static_cast<uint64_t>(sz / SECTOR_SIZE);
		fprintf(uart_out, "virtio_blk: capacity_sectors: %ld; sz: %ld\n", capacity_sectors, sz);
		fflush(uart_out);
	}

	// init queue defaults
	queue0.size			  = 0;
	queue0.desc_addr	  = 0;
	queue0.avail_addr	  = 0;
	queue0.used_addr	  = 0;
	queue0.last_avail_idx = 0;
	queue0.last_used_idx  = 0;
	queue0.ready		  = false;

	device_features_sel = 0;
	driver_features_sel = 0;
	driver_features		= 0;
	queue_sel			= 0;
	device_status		= 0;
	interrupt_status	= 0;
	config_generation	= 0;
}

std::shared_ptr<VirtIO_BLK> VirtIO_BLK::init_auto(Machine& cpu)
{
	return std::make_shared<VirtIO_BLK>(0x10001000, 0x1000, cpu, cpu.get_fdt(), cpu.get_image());
}

// DRAM memory helpers

void VirtIO_BLK::copy_from_dram(uint64_t gpa, void* dst, uint64_t len)
{
	uint8_t* out = reinterpret_cast<uint8_t*>(dst);
	for(uint64_t i = 0; i < len; ++i)
	{
		out[i] = mmap->load(gpa + i, 8);
	}
}
void VirtIO_BLK::copy_to_dram(uint64_t gpa, const void* src, uint64_t len)
{
	const uint8_t* in = reinterpret_cast<const uint8_t*>(src);
	for(uint64_t i = 0; i < len; ++i)
	{
		mmap->store(gpa + i, 8, in[i]);
	}
}

// Disk I/O
bool VirtIO_BLK::disk_read(uint64_t sector, void* buf, size_t bytes)
{
	if(!disk) return false;

	uint64_t offset = sector * SECTOR_SIZE;

	if(fseek(disk, offset, SEEK_SET) != 0)
		return false;

	size_t n = fread(buf, 1, bytes, disk);

	if(n < bytes)
	{
		memset(reinterpret_cast<uint8_t*>(buf) + n, 0, bytes - n);
	}

	return true;
}

bool VirtIO_BLK::disk_write(uint64_t sector, const void* buf, size_t bytes)
{
	if(!disk) return false;

	uint64_t offset = sector * SECTOR_SIZE;

	if(fseek(disk, offset, SEEK_SET) != 0)
		return false;

	size_t n = fwrite(buf, 1, bytes, disk);

	if(n != bytes)
		return false;

	fflush(disk);

	return true;
}

// Descriptor chain fetch
bool VirtIO_BLK::fetch_descriptor_chain(uint16_t head, std::vector<VirtqDesc>& out_chain, const VirtQueueState& q)
{
	out_chain.clear();
	if(q.desc_addr == 0) return false;
	uint16_t idx = head;
	for(uint32_t iter = 0; iter < q.size; ++iter)
	{
		uint64_t desc_addr = q.desc_addr + (uint64_t)idx * VIRTQ_DESC_SIZE;
		VirtqDesc d;
		d.addr	= (uint64_t)mmap->load(desc_addr + 0, 64);
		d.len	= (uint32_t)mmap->load(desc_addr + 8, 32);
		d.flags = (uint16_t)mmap->load(desc_addr + 12, 16);
		d.next	= (uint16_t)mmap->load(desc_addr + 14, 16);
		out_chain.push_back(d);
		if(d.flags & VIRTQ_DESC_F_NEXT)
		{
			idx = d.next;
		}
		else
		{
			return true; // chain finished normally
		}
	}
	// too many descriptors -> malformed
	return false;
}

// Queue processing
void VirtIO_BLK::process_queue(uint32_t qsel)
{
	// only queue 0 supported for virtio-blk
	if(qsel != 0) return;
	VirtQueueState& q = queue0;
	if(!q.ready || q.size == 0) return;

	// read avail.idx
	uint64_t avail_idx_addr = q.avail_addr + offsetof(VirtqAvail, idx);
	uint16_t avail_idx		= (uint16_t)mmap->load(q.avail_addr + 2, 16); // flags(2) then idx(2)

	// iterate new entries
	while(q.last_avail_idx != avail_idx)
	{
		uint16_t ring_index		 = q.last_avail_idx % q.size;
		uint64_t ring_entry_addr = q.avail_addr + 4 + (uint64_t)ring_index * 2; // ring starts at offset 4, entries are u16
		uint16_t head			 = (uint16_t)mmap->load(ring_entry_addr, 16);

		// fetch chain
		std::vector<VirtqDesc> chain;
		if(!fetch_descriptor_chain(head, chain, q))
		{
			// error: malformed chain -> write error status if possible and continue
			q.last_avail_idx++;
			continue;
		}

		// parse expected layout: [request header (read)], [data buffer(s)], [status (write 1 byte)]
		if(chain.size() < 2)
		{
			// malformed
			q.last_avail_idx++;
			continue;
		}

		// read header
		VirtioBlkReq req;
		if(chain[0].len < sizeof(VirtioBlkReq))
		{
			// malformed
			q.last_avail_idx++;
			continue;
		}
		copy_from_dram(chain[0].addr, &req, sizeof(req));

		uint8_t status_val = VIRTIO_BLK_S_IOERR;
		bool ok			   = true;
		// compute total data length from subsequent descriptors except last status descriptor
		// last descriptor expected to be status (flags WRITE and len >=1)
		// find status descriptor (should be last and VIRTQ_DESC_F_WRITE set)
		int status_desc_i  = -1;
		for(int i = (int)chain.size() - 1; i >= 1; --i)
		{
			if(chain[i].flags & VIRTQ_DESC_F_WRITE)
			{
				status_desc_i = i;
				break;
			}
		}

		if(status_desc_i < 1)
		{
			ok = false;
		}
		else
		{
			// data descriptors are from 1 .. status_desc_i-1
			size_t data_total = 0;
			for(int i = 1; i < status_desc_i; ++i)
				data_total += chain[i].len;

			// perform operation per type
			if(req.type == VIRTIO_BLK_T_IN)
			{

				// read from disk to dram
				// must read in sector-aligned chunks
				uint64_t sector	 = req.sector;
				// If multiple data descriptors, fill them sequentially
				size_t remaining = data_total;
				size_t desc_off	 = 0;
				std::vector<uint8_t> tmp_buf(data_total);
				// read from disk into tmp
				if(!disk_read(sector, tmp_buf.data(), data_total)) ok = false;
				// copy tmp into dram descriptors
				size_t pos = 0;
				for(int i = 1; i < status_desc_i && ok; ++i)
				{
					if(!(chain[i].flags & VIRTQ_DESC_F_WRITE)) ok = false;
					size_t to_copy = std::min<uint64_t>(chain[i].len, data_total - pos);
					copy_to_dram(chain[i].addr, tmp_buf.data() + pos, to_copy);
					pos += to_copy;
				}
			}
			else if(req.type == VIRTIO_BLK_T_OUT)
			{
				// write from dram into disk
				std::vector<uint8_t> tmp_buf(data_total);
				size_t pos = 0;
				for(int i = 1; i < status_desc_i; ++i)
				{
					if(pos + chain[i].len > data_total)
					{
						ok = false;
					}
					size_t to_copy = chain[i].len;
					copy_from_dram(chain[i].addr, tmp_buf.data() + pos, to_copy);
					pos += to_copy;
				}
				if(data_total % 512 != 0)
				{
					ok = false;
				}

				if(!disk_write(req.sector, tmp_buf.data(), data_total)) ok = false;
			}
			else
			{
				// unsupported type -> return I/O error (can be extended to FLUSH etc.)
				ok = false;
			}

			status_val = ok ? VIRTIO_BLK_S_OK : VIRTIO_BLK_S_IOERR;

			// write status byte into status descriptor (first byte)
			uint64_t status_addr = chain[status_desc_i].addr;
			mmap->store(status_addr, 8, status_val);
		}

		// add used element
		uint16_t used_ring_idx	= queue0.last_used_idx % q.size;
		uint64_t used_elem_addr = q.used_addr + sizeof(uint16_t) * 2 + (uint64_t)used_ring_idx * sizeof(VirtqUsedElem);
		// write id (head) and len (set to 0 or total bytes processed)
		mmap->store(used_elem_addr + 0, 32, head);
		mmap->store(used_elem_addr + 4, 32, 0); // len ignored typically

		queue0.last_used_idx++;
		// update used.idx in guest memory
		mmap->store(q.used_addr + 2, 16, queue0.last_used_idx);

		// mark that we've consumed this avail
		q.last_avail_idx++;

		// after adding used-element, raise interrupt
		interrupt_status |= 1;
		raise_irq();
	}
}

// MMIO read/write
uint64_t VirtIO_BLK::read(uint64_t addr, MemorySize size)
{
	uint64_t off = addr - start;
	switch(off)
	{
		case VIRT_REG_MAGICVALUE:
			return 0x74726976;
		case VIRT_REG_VERSION:
		case VIRT_REG_DEVICEID:
			return 0x2;
		case VIRT_REG_VENDORID:
			return 0x554d4551;
		case VIRT_REG_DEVICEFEATURES:
		{
			if(device_features_sel == 0)
				return (uint32_t)(device_features & 0xFFFFFFFF);
			else
				return (uint32_t)(device_features >> 32);
		}
		case VIRT_REG_DEVICEFEATURESSEL:
			return device_features_sel;
		case VIRT_REG_QUEUESEL:
			return queue_sel;
		case VIRT_REG_QUEUENUMMAX:
		{
			// report maximum queue size; if unset, return 0
			if(queue_sel == 0)
			{
				// choose a reasonable max (e.g., 128)
				return 128;
			}
			else
				return 0;
		}
		case VIRT_REG_QUEUENUM:
			return queue0.size;
		case VIRT_REG_QUEUEREADY:
			return queue0.ready ? 1 : 0;
		case VIRT_REG_INTERRUPTSTATUS:
			return interrupt_status;
		case VIRT_REG_STATUS:
			return device_status;
		case VIRT_REG_QUEUEDESCLOW:
			return (uint32_t)(queue0.desc_addr & 0xFFFFFFFF);
		case VIRT_REG_QUEUEDESCHIGH:
			return (uint32_t)(queue0.desc_addr >> 32);
		case VIRT_REG_QUEUEDRIVERLOW:
			return (uint32_t)(queue0.avail_addr & 0xFFFFFFFF);
		case VIRT_REG_QUEUEDRIVERHIGH:
			return (uint32_t)(queue0.avail_addr >> 32);
		case VIRT_REG_QUEUEDEVICELOW:
			return (uint32_t)(queue0.used_addr & 0xFFFFFFFF);
		case VIRT_REG_QUEUEDEVICEHIGH:
			return (uint32_t)(queue0.used_addr >> 32);
		case VIRT_REG_CONFIGGENERATION:
			return config_generation;
		case VIRT_REG_CAPACITYLOW:
			return (uint32_t)(capacity_sectors & 0xFFFFFFFF);
		case VIRT_REG_CAPACITYHIGH:
			return (uint32_t)(capacity_sectors >> 32);
		case VIRT_REG_SIZEMAX:
			return 0;
		case VIRT_REG_SEGMAX:
			return 128; // Reasonable value
		case 0x144:
			return SECTOR_SIZE;
		default:
			return 0;
	}
}

void VirtIO_BLK::write(uint64_t addr, MemorySize size, uint64_t val)
{
	uint64_t off = addr - start;
	switch(off)
	{
		case VIRT_REG_DEVICEFEATURESSEL:
			device_features_sel = (uint32_t)val;
			break;
		case VIRT_REG_DRIVERFEATURESSEL:
			driver_features_sel = (uint32_t)val;
			break;
		case VIRT_REG_DRIVERFEATURES:
		{
			if(driver_features_sel == 0)
			{
				driver_features = (driver_features & ~0xFFFFFFFFULL) | (uint64_t)(uint32_t)val;
			}
			else
			{
				driver_features = (driver_features & 0xFFFFFFFFULL) | ((uint64_t)(uint32_t)val << 32);
			}
			break;
		}
		case VIRT_REG_QUEUESEL:
			queue_sel = (uint32_t)val;
			break;
		case VIRT_REG_QUEUENUM:
			if(queue_sel == 0) queue0.size = (uint32_t)val;
			break;
		case VIRT_REG_QUEUEREADY:
			if(queue_sel == 0) queue0.ready = (val != 0);
			break;
		case VIRT_REG_QUEUEDESCLOW:
			if(queue_sel == 0) queue0.desc_addr = (queue0.desc_addr & ~0xFFFFFFFFULL) | (uint64_t)(uint32_t)val;
			break;
		case VIRT_REG_QUEUEDESCHIGH:
			if(queue_sel == 0) queue0.desc_addr = (queue0.desc_addr & 0xFFFFFFFFULL) | ((uint64_t)(uint32_t)val << 32);
			break;
		case VIRT_REG_QUEUEDRIVERLOW:
			if(queue_sel == 0) queue0.avail_addr = (queue0.avail_addr & ~0xFFFFFFFFULL) | (uint64_t)(uint32_t)val;
			break;
		case VIRT_REG_QUEUEDRIVERHIGH:
			if(queue_sel == 0) queue0.avail_addr = (queue0.avail_addr & 0xFFFFFFFFULL) | ((uint64_t)(uint32_t)val << 32);
			break;
		case VIRT_REG_QUEUEDEVICELOW:
			if(queue_sel == 0) queue0.used_addr = (queue0.used_addr & ~0xFFFFFFFFULL) | (uint64_t)(uint32_t)val;
			break;
		case VIRT_REG_QUEUEDEVICEHIGH:
			if(queue_sel == 0) queue0.used_addr = (queue0.used_addr & 0xFFFFFFFFULL) | ((uint64_t)(uint32_t)val << 32);
			break;
		case VIRT_REG_QUEUENOTIFY:
		{
			// guest notifies device: process queue
			uint32_t q = (uint32_t)val;
			process_queue(q);
			break;
		}
		case VIRT_REG_INTERRUPTACK:
			// guest writes bits to clear
			interrupt_status &= ~((uint32_t)val);
			break;
		case VIRT_REG_STATUS:
			if(val == 0)
			{
				device_status	 = 0;
				interrupt_status = 0;

				queue0.ready	 = false;
				queue0.size		 = 0;
				queue0.desc_addr = queue0.avail_addr = queue0.used_addr = 0;
				queue0.last_avail_idx = queue0.last_used_idx = 0;
			}
			else
			{
				device_status |= (uint32_t)val;

				if(device_status & VIRT_STATUS_FEATURES_OK)
				{
					if((driver_features & ~device_features) != 0)
					{
						device_status &= ~VIRT_STATUS_FEATURES_OK;
					}
				}
			}
			break;
		default:
			// ignore writes to other offsets for now
			break;
	}
}

void VirtIO_BLK::raise_irq()
{
	plic->set_pending(irq_num, true);
}
