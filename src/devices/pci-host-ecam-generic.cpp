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

#include "../../include/devices/pci/pci-host-ecam-generic.hpp"
#include "../../include/devices/plic.hpp"
#include "../../include/machine.hpp"
namespace rv64vm::dev
{
	PCI_HEG::PCI_HEG(uint64_t base, runner::Machine& cpu, fdt_node* fdt)
		: Device(base, 0x08000000, fdt, cpu.get_mmap()), cpu(cpu), plic(cpu.get_mmio()->get<PLIC>().get())
	{
		cpu.get_mmap()->add_region(start, size);

		// Remove all 32 devs
		std::fill(std::begin(devices), std::end(devices), nullptr);

		if(fdt != NULL)
		{
			uint32_t io_cpu_addr   = base + 0x00100000; // 0x30100000
			uint32_t mmio_cpu_addr = base + 0x00200000; // 0x30200000

			struct fdt_node* pci_fdt = fdt_node_create_reg("pci", base);

			fdt_node_add_prop_reg(pci_fdt, "reg", base, 0x00100000);

			fdt_node_add_prop_str(pci_fdt, "compatible", "pci-host-ecam-generic");
			fdt_node_add_prop_str(pci_fdt, "device_type", "pci");
			fdt_node_add_prop_u32(pci_fdt, "#address-cells", 3);
			fdt_node_add_prop_u32(pci_fdt, "#size-cells", 2);
			fdt_node_add_prop_u32(pci_fdt, "#interrupt-cells", 1);
			fdt_node_add_prop_cells(pci_fdt, "bus-range", { 0, 0 }, 2);
			fdt_node* soc	 = fdt_node_find(fdt, "soc");
			fdt_node* plicfd = fdt_node_find_reg(soc, "plic", 0x0C000000);
			uint32_t phandle = fdt_node_get_phandle(plicfd);
			fdt_node_add_prop_u32(pci_fdt, "interrupt-parent", phandle);
			fdt_node_free(plicfd);

			std::vector<uint32_t> interrupt_map_mask = { 0xf800, 0x0, 0x0, 0x7 };
			fdt_node_add_prop_cells(pci_fdt, "interrupt-map-mask", interrupt_map_mask, interrupt_map_mask.size());

			std::vector<uint32_t> interrupt_map;
			for(uint32_t slot = 0; slot < 32; slot++)
			{
				uint32_t devfn_addr = (slot << 11); // PCI address encoding: bus=0, device=slot, func=0 -> bits [15:11]=devno
				uint32_t irq_num	= plic->last_irq();

				interrupt_map.insert(interrupt_map.end(), { devfn_addr, 0x0, 0x0, // PCI unit address
															0x1,				  // INTA#
															phandle,
															irq_num });
			}
			fdt_node_add_prop_cells(pci_fdt, "interrupt-map", interrupt_map, interrupt_map.size());

			std::vector<uint32_t> dynamic_ranges = {
				// IO Window
				0x01000000,
				0x0,
				0x00000000,
				0x0,
				io_cpu_addr,
				0x0,
				0x00010000,
				// Non-prefetchable MMIO32 window
				0x02000000,
				0x0,
				mmio_cpu_addr,
				0x0,
				mmio_cpu_addr,
				0x0,
				0x07e00000,
			};

			fdt_node_add_prop_cells(pci_fdt, "ranges", dynamic_ranges, dynamic_ranges.size());
			fdt_node_add_child(soc, pci_fdt);
			fdt_node_free(soc);
		}
	}

	std::shared_ptr<PCI_HEG> PCI_HEG::init_auto(runner::Machine& cpu)
	{
		return std::make_shared<PCI_HEG>(0x30000000, cpu, cpu.get_fdt());
	}

	void PCI_HEG::attach_device(uint8_t slot, PCI_Device* dev)
	{
		if(slot < 32)
		{
			devices[slot] = dev;
		}
	}

	uint64_t PCI_HEG::read(uint64_t addr, MemorySize size)
	{
		uint64_t offset = addr - start;

		// ECAM (first megabyte 0x00000000 - 0x000FFFFF)
		if(offset < 0x00100000)
		{
			uint32_t bus  = (offset >> 20) & 0xFF; // always zero, since we have only 1 bus
			uint32_t dev  = (offset >> 15) & 0x1F; // slot number (0-31)
			uint32_t func = (offset >> 12) & 0x07; // slot function (normally 0)
			uint32_t reg  = offset & 0xFFF;		   // Byte offset inside config

			if(devices[dev] != nullptr && func == 0)
			{
				return devices[dev]->read_config(reg, size);
			}
			return 0xFFFFFFFF; // if no device, return all ones
		}

		// MMIO (from 2 MB to region end 128 MB)
		if(offset >= 0x00200000 && offset < 0x08000000)
		{
			for(auto* dev : devices)
			{
				if(!dev) continue;
				for(int bar_idx = 0; bar_idx < 6; bar_idx++)
				{
					uint64_t bar_start = dev->bars[bar_idx].address;
					uint64_t bar_size  = dev->bars[bar_idx].size;

					// We check whether a BAR address has been assigned by the guest and whether the current system address falls within this range.
					if(bar_size > 0 && bar_start > 0 && addr >= bar_start && addr < (bar_start + bar_size))
					{
						return dev->read_mmio(addr - bar_start, size);
					}
				}
			}
		}

		return 0;
	}

	void PCI_HEG::write(uint64_t addr, MemorySize size, uint64_t val)
	{
		uint64_t offset = addr - start;

		// ECAM
		if(offset < 0x00100000)
		{
			uint32_t bus  = (offset >> 20) & 0xFF;
			uint32_t dev  = (offset >> 15) & 0x1F;
			uint32_t func = (offset >> 12) & 0x07;
			uint32_t reg  = offset & 0xFFF;

			if(devices[dev] != nullptr && func == 0)
			{
				devices[dev]->write_config(reg, size, val);
			}
			return;
		}

		// MMIO
		if(offset >= 0x00200000 && offset < 0x08000000)
		{
			for(auto* dev : devices)
			{
				if(!dev) continue;
				for(int bar_idx = 0; bar_idx < 6; bar_idx++)
				{
					uint64_t bar_start = dev->bars[bar_idx].address;
					uint64_t bar_size  = dev->bars[bar_idx].size;

					if(bar_size > 0 && bar_start > 0 && addr >= bar_start && addr < (bar_start + bar_size))
					{
						dev->write_mmio(addr - bar_start, size, val);
						return;
					}
				}
			}
		}
	}
}
