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

#include "../../../include/devices/virtio/virtio_rng.hpp"
#include "../../../include/machine.hpp"
#include "../../../include/utils/random.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <sys/stat.h>

namespace rv64vm::dev
{
	VirtIO_RNG::VirtIO_RNG(uint64_t base, uint64_t size, rv64vm::runner::Machine& cpu, fdt_node* fdt)
		: Device(base, size, fdt, cpu.get_mmap()), plic(cpu.get_mmio()->get<PLIC>().get()), irq_num(plic->acquire_irq())
	{
		cpu.get_mmap()->add_region(base, size);

		if(fdt != nullptr)
		{
			fdt_node* virtio_rng_node = fdt_node_create_reg("virtio_mmio", base);
			fdt_node_add_prop(virtio_rng_node, "compatible", "virtio,mmio\0", 12);
			fdt_node_add_prop_reg(virtio_rng_node, "reg", base, size);
			fdt_node* soc  = fdt_node_find(fdt, "soc");
			fdt_node* plic = fdt_node_find_reg(soc, "plic", 0x0C000000);
			fdt_node_add_prop_u32(virtio_rng_node, "interrupt-parent", fdt_node_get_phandle(plic));
			fdt_node_free(plic);
			fdt_node_add_prop_u32(virtio_rng_node, "interrupts", irq_num);
			fdt_node_add_child(soc, virtio_rng_node);
			fdt_node_free(soc);
		}
		// Device features
		device_features = VIRTIO_F_VERSION_1;

		FILE* uart_out = cpu.get_uart_output();

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

		virtio_rng_seed(util_urandom(0, UINT64_MAX), util_urandom(0, UINT64_MAX));

		fprintf(uart_out, "virtio_rng: state: 0x%llx; inc: 0x%llx\n", rng_ctx.state, rng_ctx.inc);
	}

	std::shared_ptr<VirtIO_RNG> VirtIO_RNG::init_auto(runner::Machine& cpu)
	{
		cpu.virtio_count++;
		// TODO: Rework this after PCI will appear since it can contain unlimited devices
		assert(cpu.virtio_count <= 8 && "You cannot have more than 8 VirtIO devices at the same time!");
		return std::make_shared<VirtIO_RNG>(0x10000000 + 0x1000 * cpu.virtio_count, 0x1000, cpu, cpu.get_fdt());
	}

	// DRAM memory helpers

	void VirtIO_RNG::copy_from_dram(uint64_t gpa, void* dst, uint64_t len)
	{
		uint8_t* out = reinterpret_cast<uint8_t*>(dst);
		for(uint64_t i = 0; i < len; ++i)
		{
			out[i] = mmap->load(gpa + i, 8);
		}
	}
	void VirtIO_RNG::copy_to_dram(uint64_t gpa, const void* src, uint64_t len)
	{
		const uint8_t* in = reinterpret_cast<const uint8_t*>(src);
		for(uint64_t i = 0; i < len; ++i)
		{
			mmap->store(gpa + i, 8, in[i]);
		}
	}

	// Descriptor chain fetch
	bool VirtIO_RNG::fetch_descriptor_chain(uint16_t head, std::vector<VirtqDesc>& out_chain, const VirtQueueState& q)
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
	void VirtIO_RNG::process_queue(uint32_t qsel)
	{
		if(qsel != 0) return;
		VirtQueueState& q = queue0;
		if(!q.ready || q.size == 0) return;

		uint16_t avail_idx = (uint16_t)mmap->load(q.avail_addr + 2, 16);

		while(q.last_avail_idx != avail_idx)
		{
			uint16_t ring_index = q.last_avail_idx % q.size;
			uint16_t head		= (uint16_t)mmap->load(q.avail_addr + 4 + ring_index * 2, 16);

			std::vector<VirtqDesc> chain;
			if(!fetch_descriptor_chain(head, chain, q))
			{
				q.last_avail_idx++;
				continue;
			}

			uint32_t total_written = 0;

			for(const auto& d : chain)
			{
				if(!(d.flags & VIRTQ_DESC_F_WRITE)) continue;

				std::vector<uint8_t> buf(d.len);
				virtio_rng_fill_buffer(buf.data(), d.len);
				copy_to_dram(d.addr, buf.data(), d.len);
				total_written += d.len;
			}

			uint16_t used_idx  = q.last_used_idx % q.size;
			uint64_t used_elem = q.used_addr + 4 + used_idx * 8;
			mmap->store(used_elem + 0, 32, head);
			mmap->store(used_elem + 4, 32, total_written);

			q.last_used_idx++;
			mmap->store(q.used_addr + 2, 16, q.last_used_idx);

			q.last_avail_idx++;
			interrupt_status |= 1;
			raise_irq();
		}
	}

	// MMIO read/write
	uint64_t VirtIO_RNG::read(uint64_t addr, MemorySize size)
	{
		uint64_t off = addr - start;
		switch(off)
		{
			case VIRT_REG_MAGICVALUE:
				return 0x74726976;
			case VIRT_REG_VERSION:
				return 0x2;
			case VIRT_REG_DEVICEID:
				return 0x4;
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
			default:
				return 0;
		}
	}

	void VirtIO_RNG::write(uint64_t addr, MemorySize size, uint64_t val)
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

	void VirtIO_RNG::raise_irq()
	{
		plic->set_pending(irq_num, true);
	}
}
