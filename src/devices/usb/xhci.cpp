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
#include "../../../include/devices/usb/xhci.hpp"
#include "../../../include/devices/plic.hpp"
#include "../../../include/machine.hpp"

namespace rv64vm::dev
{
	XHCI::XHCI(uint64_t start, runner::Machine& cpu, fdt_node* fdt)
		: Device(start, 0x2000, fdt, cpu.get_mmap()),
		  cpu(cpu),
		  plic(cpu.get_mmio()->get<PLIC>().get()),
		  irq_num(plic->acquire_irq())
	{
		cpu.get_mmap()->add_region(start, size);

		if(fdt != nullptr)
		{
			fdt_node* soc = fdt_node_find(fdt, "soc");
			fdt_node* clk = fdt_node_find(fdt, "xhci_clk");
			if(!clk)
			{
				struct fdt_node* clk_fdt = fdt_node_create("xhci_clk");
				fdt_node_add_prop_str(clk_fdt, "compatible", "fixed-clock");
				fdt_node_add_prop_u32(clk_fdt, "#clock-cells", 0);
				fdt_node_add_prop_u32(clk_fdt, "clock-frequency", 24000000);
				fdt_node_add_child(fdt, clk_fdt);
			}
			fdt_node_free(clk);
			clk = fdt_node_find(fdt, "xhci_clk");
			fdt_node_get_phandle(clk);

			struct fdt_node* usb_fdt = fdt_node_create_reg("usb", start);
			fdt_node_add_prop_reg(usb_fdt, "reg", start, size);
			fdt_node_add_prop_str(usb_fdt, "compatible", "generic-xhci");

			fdt_node* plic = fdt_node_find_reg(soc, "plic", 0x0C000000);
			fdt_node_add_prop_u32(usb_fdt, "interrupt-parent", fdt_node_get_phandle(plic));
			fdt_node_add_prop_u32(usb_fdt, "interrupts", irq_num);
			fdt_node_add_prop(usb_fdt, "dma-coherent", NULL, 0);
			fdt_node_add_prop_str(usb_fdt, "status", "okay");

			fdt_node_add_prop_u32(usb_fdt, "clocks", fdt_node_get_phandle(clk));
			fdt_node_free(clk);

			fdt_node_add_child(soc, usb_fdt);
			fdt_node_free(soc);
		}

		reset();
	}

	std::shared_ptr<XHCI> XHCI::init_auto(runner::Machine& cpu)
	{
		return std::make_shared<XHCI>(0x100a0000, cpu, cpu.get_fdt());
	}

	void XHCI::reset()
	{
		cmd.raw				= 0;
		sts.raw				= 0;
		sts.fields.hchalted = 1;
		sts.fields.cnr		= 0;

		config_reg = 0;
		dcbaap	   = 0;
		crcr_base  = 0;

		for(uint32_t i = 0; i < XHCI_MAX_PORTS; ++i)
		{
			port_sc[i].raw			= 0;
			port_sc[i].fields.pp	= 1; // Power on
			port_sc[i].fields.speed = 4; // SuperSpeed
			port_sc[i].fields.pls	= 5; // RxDetect
		}

		interrupters[0] = {};
	}

	void XHCI::update_irq()
	{
		bool pending = sts.fields.eint && (interrupters[0].iman & 0x1) && cmd.fields.inte;
		plic->set_pending(irq_num, pending);
	}

	uint64_t XHCI::read(uint64_t addr, MemorySize size)
	{
		uint64_t offs = addr - start;

		// Capability Registers
		if(offs < XHCI_CAPLENGTH)
		{
			switch(offs)
			{
				case 0x00: // CAPLENGTH & HCIVERSION
					return XHCI_CAPLENGTH | (static_cast<uint32_t>(XHCI_HCIVERSION) << 16);
				case 0x04: // HCSPARAMS1 (MaxPorts, MaxIntrs, MaxSlots)
					return (static_cast<uint32_t>(XHCI_MAX_PORTS) << 24) | (1 << 8) | 8;
				case 0x08: // HCSPARAMS2
					return 0;
				case 0x0C: // HCSPARAMS3
					return 0;
				case 0x10: // HCCPARAMS1 (64-bit addressing, CSZ=1)
					return (1 << 2) | 1;
				case 0x14: // DBOFF
					return XHCI_DBOFF;
				case 0x18: // RTSOFF
					return XHCI_RTSOFF;
				default:
					return 0;
			}
		}

		// Operational Registers
		if(offs >= XHCI_CAPLENGTH && offs < XHCI_RTSOFF)
		{
			uint64_t op = offs - XHCI_CAPLENGTH;
			switch(op)
			{
				case 0x00: // USBCMD
					return cmd.raw;
				case 0x04: // USBSTS
					return sts.raw;
				case 0x08: // PAGESIZE (4KB)
					return 1;
				case 0x18: // CRCR (Low 32)
					return static_cast<uint32_t>(crcr_base);
				case 0x1C: // CRCR (High 32)
					return static_cast<uint32_t>(crcr_base >> 32);
				case 0x30: // DCBAAP (Low 32)
					return static_cast<uint32_t>(dcbaap);
				case 0x34: // DCBAAP (High 32)
					return static_cast<uint32_t>(dcbaap >> 32);
				case 0x38: // CONFIG
					return config_reg;
				default:
					// PORTSC array (Base + 0x400)
					if(op >= 0x400 && op < 0x400 + (XHCI_MAX_PORTS * 0x10))
					{
						uint32_t idx = (op - 0x400) / 0x10;
						return port_sc[idx].raw;
					}
					return 0;
			}
		}

		// Runtime Registers (Interrupter 0 at RTSOFF + 0x20)
		if(offs >= XHCI_RTSOFF && offs < XHCI_DBOFF)
		{
			uint64_t rt = offs - XHCI_RTSOFF;
			if(rt >= 0x20 && rt < 0x40)
			{
				switch(rt - 0x20)
				{
					case 0x00:
						return interrupters[0].iman;
					case 0x04:
						return interrupters[0].imod;
					case 0x08:
						return interrupters[0].erstsz;
					case 0x10:
						return static_cast<uint32_t>(interrupters[0].erstba);
					case 0x14:
						return static_cast<uint32_t>(interrupters[0].erstba >> 32);
					case 0x18:
						return static_cast<uint32_t>(interrupters[0].erdp);
					case 0x1C:
						return static_cast<uint32_t>(interrupters[0].erdp >> 32);
					default:
						return 0;
				}
			}
			return 0;
		}

		return 0;
	}

	void XHCI::write(uint64_t addr, MemorySize size, uint64_t val)
	{
		uint64_t offs = addr - start;

		if(offs < XHCI_CAPLENGTH) return;

		// Operational Registers
		if(offs >= XHCI_CAPLENGTH && offs < XHCI_RTSOFF)
		{
			uint64_t op = offs - XHCI_CAPLENGTH;
			switch(op)
			{
				case 0x00: // USBCMD
				{
					cmd.raw = static_cast<uint32_t>(val);

					if(cmd.fields.hcrst)
					{
						reset();
						cmd.fields.hcrst = 0;
					}

					sts.fields.hchalted = !cmd.fields.runstop;
					update_irq();
					break;
				}
				case 0x04: // USBSTS (RW1C)
					sts.raw &= ~static_cast<uint32_t>(val);
					update_irq();
					break;
				case 0x18: // CRCR Low
					crcr_base	 = (crcr_base & 0xFFFFFFFF00000000ULL) | static_cast<uint32_t>(val);
					crcr_dequeue = crcr_base & ~0x3FULL;
					pcs			 = crcr_base & 1; // RCS bit
					break;
				case 0x1C: // CRCR High
					crcr_base	 = (crcr_base & 0xFFFFFFFFULL) | (val << 32);
					crcr_dequeue = crcr_base & ~0x3FULL;
					pcs			 = crcr_base & 1;
					break;
				case 0x30: // DCBAAP Low
					dcbaap = (dcbaap & 0xFFFFFFFF00000000ULL) | static_cast<uint32_t>(val);
					break;
				case 0x34: // DCBAAP High
					dcbaap = (dcbaap & 0xFFFFFFFFULL) | (val << 32);
					break;
				case 0x38: // CONFIG
					config_reg = static_cast<uint32_t>(val) & 0xFF;
					break;
				default:
					if(op >= 0x400 && op < 0x400 + (XHCI_MAX_PORTS * 0x10))
					{
						uint32_t idx  = (op - 0x400) / 0x10;
						uint32_t wval = static_cast<uint32_t>(val);

						// Preserve RW1S/RO fields, clear RW1C on '1' write
						if(wval & (1 << 4)) // PR (Port Reset)
						{
							port_sc[idx].fields.prc = 1;
							port_sc[idx].fields.ped = 1;
							port_sc[idx].fields.pls = 0; // U0
						}

						// RW1C bits handling
						uint32_t rw1c_mask = (1 << 17) | (1 << 18) | (1 << 21) | (1 << 22);
						port_sc[idx].raw &= ~(wval & rw1c_mask);
					}
					break;
			}
			return;
		}

		// Runtime Registers (Interrupter 0)
		if(offs >= XHCI_RTSOFF && offs < XHCI_DBOFF)
		{
			uint64_t rt = offs - XHCI_RTSOFF;
			if(rt >= 0x20 && rt < 0x40)
			{
				switch(rt - 0x20)
				{
					case 0x00: // IMAN
						interrupters[0].iman = static_cast<uint32_t>(val);
						update_irq();
						break;
					case 0x04: // IMOD
						interrupters[0].imod = static_cast<uint32_t>(val);
						break;
					case 0x08: // ERSTSZ
						interrupters[0].erstsz = static_cast<uint32_t>(val) & 0xFFFF;
						break;
					case 0x10: // ERSTBA Low
						interrupters[0].erstba = (interrupters[0].erstba & 0xFFFFFFFF00000000ULL) | static_cast<uint32_t>(val);
						break;
					case 0x14: // ERSTBA High
						interrupters[0].erstba = (interrupters[0].erstba & 0xFFFFFFFFULL) | (val << 32);
						break;
					case 0x18: // ERDP Low
						interrupters[0].erdp = (interrupters[0].erdp & 0xFFFFFFFF00000000ULL) | static_cast<uint32_t>(val);
						break;
					case 0x1C: // ERDP High
						interrupters[0].erdp = (interrupters[0].erdp & 0xFFFFFFFFULL) | (val << 32);
						break;
				}
			}
			return;
		}

		// Doorbell Registers
		if(offs >= XHCI_DBOFF)
		{
			uint32_t slot_id = (offs - XHCI_DBOFF) / 4;
			uint32_t target	 = static_cast<uint32_t>(val) & 0xFF;

			if(slot_id == 0 && target == 0)
			{
				// Command Ring Doorbell triggered
				process_command_ring();
			}
		}
	}

	trb_t XHCI::read_trb(uint64_t addr)
	{
		trb_t trb{};
		uint8_t* ptr = reinterpret_cast<uint8_t*>(&trb);
		for(size_t i = 0; i < sizeof(trb_t); ++i)
		{
			ptr[i] = static_cast<uint8_t>(cpu.get_mmap()->load(addr + i, 8));
		}
		return trb;
	}

	void XHCI::write_trb(uint64_t addr, const trb_t& trb)
	{
		const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&trb);
		for(size_t i = 0; i < sizeof(trb_t); ++i)
		{
			cpu.get_mmap()->store(addr + i, 8, ptr[i]);
		}
	}

	void XHCI::push_event(const trb_t& evt)
	{
		if(!interrupters[0].erstba || !interrupters[0].erstsz) return;

		// Read Segment 0 from ERST
		erst_entry_t erst{};
		uint8_t* erst_ptr = reinterpret_cast<uint8_t*>(&erst);
		for(size_t i = 0; i < sizeof(erst_entry_t); ++i)
		{
			erst_ptr[i] = static_cast<uint8_t>(cpu.get_mmap()->load(interrupters[0].erstba + i, 8));
		}

		if(!erst.ring_segment_base) return;

		// Write event TRB to current ERDP
		uint64_t erdp				 = interrupters[0].erdp & ~0xFULL;
		trb_t out_evt				 = evt;
		out_evt.control.fields.cycle = event_pcs;

		write_trb(erdp, out_evt);

		// Advance ERDP pointer
		erdp += sizeof(trb_t);
		uint64_t ring_end = erst.ring_segment_base + (erst.ring_segment_size * sizeof(trb_t));
		if(erdp >= ring_end)
		{
			erdp	  = erst.ring_segment_base;
			event_pcs = !event_pcs; // Toggle cycle state on wrap
		}

		interrupters[0].erdp = erdp | (interrupters[0].erdp & 0xFUL);

		// Trigger IRQ
		sts.fields.eint = 1;
		update_irq();
	}

	void XHCI::process_command_ring()
	{
		while(true)
		{
			trb_t trb = read_trb(crcr_dequeue);

			// Check if TRB is owned by HC
			if(trb.control.fields.cycle != pcs)
			{
				break;
			}

			// Handle Link TRB
			if(trb.get_type() == TRBType::LINK)
			{
				crcr_dequeue = trb.parameter & ~0xFULL;
				if(trb.control.fields.ent) // Toggle Cycle bit
				{
					pcs = !pcs;
				}
				continue;
			}

			trb_t evt{};
			evt.parameter			= crcr_dequeue; // Pointer to original command TRB
			evt.status				= (static_cast<uint32_t>(TRBCompletionCode::SUCCESS) << 24);
			evt.control.fields.type = static_cast<uint32_t>(TRBType::CMD_COMPLETION_EVENT);

			switch(trb.get_type())
			{
				case TRBType::NO_OP_CMD:
					push_event(evt);
					break;

				case TRBType::ENABLE_SLOT:
					evt.control.fields.control = (1 << 8); // Assign Slot ID = 1
					push_event(evt);
					break;

				case TRBType::DISABLE_SLOT:
				case TRBType::ADDRESS_DEVICE:
				case TRBType::CONFIG_ENDPOINT:
				case TRBType::EVAL_CONTEXT:
					push_event(evt);
					break;

				default:
					evt.status = (static_cast<uint32_t>(TRBCompletionCode::TRB_ERROR) << 24);
					push_event(evt);
					break;
			}

			crcr_dequeue += sizeof(trb_t);
		}
	}
}
