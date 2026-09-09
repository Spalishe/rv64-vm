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
	XHCI::XHCI(runner::Machine& cpu)
		: PCI_Device(0x1836, 0x000D, 0x0C),
		  cpu(cpu),
		  plic(cpu.get_mmio()->get<PLIC>().get()),
		  irq_num(plic->acquire_irq())
	{
		write_config_fast<uint8_t>(0x0A, 0x03); // Subclass
		write_config_fast<uint8_t>(0x09, 0x30); // Programming Interface (XHCI)

		uint64_t xhci_mmio_size = 0x00010000;
		init_bar(0, xhci_mmio_size, 0x02);

		reset();
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
		event_ring_pcs	= true;
	}

	void XHCI::update_irq()
	{
		bool pending = sts.fields.eint && (interrupters[0].iman & 0x1) && cmd.fields.inte;
		plic->set_pending(irq_num, pending);
	}

	uint64_t XHCI::read_mmio(uint64_t offs, MemorySize size)
	{
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
				case 0x10:	  // HCCPARAMS1 (64-bit addressing, CSZ=1)
					return 1; // (1 << 2) for CSZ
				case 0x14:	  // DBOFF
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
	void XHCI::write_mmio(uint64_t offs, MemorySize size, uint64_t val)
	{
		if(offs < XHCI_CAPLENGTH) return;
		printf("XHCI write offs: 0x%llx size: %d val: 0x%llx\n", offs, (int)size, val);
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
					pcs			 = crcr_base & 1;
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

						auto& port = port_sc[idx];

						uint32_t rw1c_mask = (1 << 17) | (1 << 18) | (1 << 21);
						port.raw &= ~(wval & rw1c_mask);

						if((wval & (1 << 4)) && ports[idx])
						{
							port.fields.pr	= 0;
							port.fields.ped = 1;
							port.fields.prc = 1;
						}
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
					case 0x00:
						interrupters[0].iman = static_cast<uint32_t>(val);
						update_irq();
						break;
					case 0x04:
						interrupters[0].imod = static_cast<uint32_t>(val);
						break;
					case 0x08:
						interrupters[0].erstsz = static_cast<uint32_t>(val) & 0xFFFF;
						break;
					case 0x10:
						interrupters[0].erstba = (interrupters[0].erstba & 0xFFFFFFFF00000000ULL) | static_cast<uint32_t>(val);
						break;
					case 0x14:
						interrupters[0].erstba = (interrupters[0].erstba & 0xFFFFFFFFULL) | (val << 32);
						update_event_ring_segment(0);
						break;
					case 0x18:
						interrupters[0].erdp = (interrupters[0].erdp & 0xFFFFFFFF00000000ULL) | static_cast<uint32_t>(val);
						break;
					case 0x1C:
						interrupters[0].erdp = (interrupters[0].erdp & 0xFFFFFFFFULL) | (val << 32);
						break;
				}
			}
			return;
		}

		// Doorbell Registers FIX
		if(offs >= XHCI_DBOFF)
		{
			write_doorbell(static_cast<uint32_t>(offs - XHCI_DBOFF), static_cast<uint32_t>(val));
		}
	}

	void XHCI::read_dma(uint64_t addr, void* dest, size_t size)
	{
		uint8_t* ptr = static_cast<uint8_t*>(dest);
		for(size_t i = 0; i < size; ++i)
		{
			ptr[i] = static_cast<uint8_t>(cpu.get_mmap()->load(addr + i, 8));
		}
	}

	void XHCI::write_dma(uint64_t addr, const void* src, size_t size)
	{
		const uint8_t* ptr = static_cast<const uint8_t*>(src);
		for(size_t i = 0; i < size; ++i)
		{
			cpu.get_mmap()->store(addr + i, 8, ptr[i]);
		}
	}

	trb_t XHCI::read_trb(uint64_t addr)
	{
		trb_t trb{};
		read_dma(addr, &trb, sizeof(trb_t));
		return trb;
	}

	void XHCI::write_trb(uint64_t addr, const trb_t& trb)
	{
		write_dma(addr, &trb, sizeof(trb_t));
	}

	uint64_t XHCI::get_ep_ctx_tr_enqueue_pointer(uint32_t slot_id, uint32_t ep_index)
	{
		if(ep_index < 1 || ep_index > 31) return 0;

		auto it = slots.find(slot_id);
		if(it == slots.end() || !it->second) return 0;

		if(!dcbaap) return 0;

		uint64_t slot_dc_ptr = 0;
		read_dma(dcbaap + (slot_id * sizeof(uint64_t)), &slot_dc_ptr, sizeof(slot_dc_ptr));
		if(!slot_dc_ptr) return 0;

		constexpr size_t ctx_size = 32;
		uint64_t ep_ctx_ptr		  = slot_dc_ptr + (ep_index * ctx_size);

		uint64_t tr_dequeue = 0;
		read_dma(ep_ctx_ptr + 0x08, &tr_dequeue, sizeof(tr_dequeue));

		return tr_dequeue & ~0xFULL;
	}

	void XHCI::send_transfer_event(uint32_t slot_id, uint32_t ep_index, TRBCompletionCode code)
	{
		trb_t evt{};
		evt.status				   = (static_cast<uint32_t>(code) << 24);
		evt.control.fields.type	   = static_cast<uint32_t>(TRBType::TRANSFER_EVENT);
		evt.control.fields.control = (slot_id << 8) | (ep_index & 0x1F);
		push_event(evt);
	}

	void XHCI::write_doorbell(uint32_t offset, uint32_t val)
	{
		uint8_t slot_id	 = offset / 4;
		uint8_t ep_index = val & 0xFF;

		if(slot_id == 0 && ep_index == 0)
		{
			process_command_ring();
			return;
		}

		if(ep_index == 1) // Control Endpoint (EP0)
		{
			uint64_t ep0_ring_dma_addr = get_ep_ctx_tr_enqueue_pointer(slot_id, ep_index);
			if(ep0_ring_dma_addr != 0)
			{
				process_ep0_transfer_ring(slot_id, ep0_ring_dma_addr);
			}
		}
	}
	inline bool trb_has_ioc(const trb_t& trb)
	{
		return (trb.control.fields.flags & 0x08) != 0;
	}
	void XHCI::process_ep0_transfer_ring(uint32_t slot_id, uint64_t trb_dma_addr)
	{
		auto it = slots.find(slot_id);
		if(it == slots.end() || !it->second || !it->second->attached_device) return;
		auto dev = it->second->attached_device;

		usb_setup_packet_t setup_pkt{};

		while(trb_dma_addr != 0)
		{
			trb_t trb = read_trb(trb_dma_addr);

			// Проверка флага цикла (Cycle Bit)
			if(trb.control.fields.cycle != slots[slot_id]->ep0_pcs) break;

			TRBType type = trb.get_type();

			if(type == TRBType::LINK)
			{
				trb_dma_addr = trb.parameter & ~0xFULL;
				if(trb.control.fields.ent)
				{
					slots[slot_id]->ep0_pcs = !slots[slot_id]->ep0_pcs;
				}
				continue;
			}

			switch(type)
			{
				case TRBType::SETUP_STAGE:
				{
					std::memcpy(&setup_pkt, &trb.parameter, sizeof(usb_setup_packet_t));
					break;
				}
				case TRBType::DATA_STAGE:
				{
					bool is_read		 = (setup_pkt.bmRequestType & 0x80) != 0;
					uint32_t trb_buf_len = trb.status & 0x1FFFF; // host allocated buffer size

					if(is_read)
					{
						auto resp		   = dev->handle_control_request(setup_pkt);
						uint32_t requested = std::min<uint32_t>(setup_pkt.wLength, trb_buf_len);
						uint32_t to_copy   = std::min<uint32_t>(resp.size(), requested);

						if(to_copy > 0)
							write_dma(trb.parameter, resp.data(), to_copy);

						bool short_packet = to_copy < trb_buf_len;
						if(trb_has_ioc(trb) || short_packet)
						{
							trb_t evt{};
							evt.parameter			   = trb_dma_addr;
							uint32_t residual		   = trb_buf_len - to_copy;
							TRBCompletionCode cc	   = short_packet ? TRBCompletionCode::SHORT_PACKET : TRBCompletionCode::SUCCESS;
							evt.status				   = residual | (static_cast<uint32_t>(cc) << 24);
							evt.control.fields.type	   = static_cast<uint32_t>(TRBType::TRANSFER_EVENT);
							evt.control.fields.control = (slot_id << 8) | (1 & 0x1F);
							push_event(evt);
						}
					}
					else
					{
						uint32_t len = std::min<uint32_t>(trb_buf_len, setup_pkt.wLength);
						std::vector<uint8_t> data(len);
						read_dma(trb.parameter, data.data(), data.size());
						dev->handle_control_data_out(setup_pkt, data);
					}
					break;
				}
				case TRBType::STATUS_STAGE:
				{
					trb_t evt{};
					evt.parameter			   = trb_dma_addr;
					evt.status				   = static_cast<uint32_t>(TRBCompletionCode::SUCCESS) << 24;
					evt.control.fields.type	   = static_cast<uint32_t>(TRBType::TRANSFER_EVENT);
					evt.control.fields.control = (slot_id << 8) | (1 & 0x1F);
					push_event(evt);
					break;
				}
				default:
					break;
			}

			trb_dma_addr += sizeof(trb_t);
			it->second->ep0_tr_dequeue = trb_dma_addr;
		}
	}

	void XHCI::process_command_ring()
	{
		while(true)
		{
			trb_t trb = read_trb(crcr_dequeue);

			if(trb.control.fields.cycle != pcs) break;

			if(trb.get_type() == TRBType::LINK)
			{
				crcr_dequeue = trb.parameter & ~0xFULL;
				if(trb.control.fields.ent) pcs = !pcs;
				continue;
			}

			trb_t evt{};
			evt.parameter			= crcr_dequeue;
			evt.status				= (static_cast<uint32_t>(TRBCompletionCode::SUCCESS) << 24);
			evt.control.fields.type = static_cast<uint32_t>(TRBType::CMD_COMPLETION_EVENT);

			switch(trb.get_type())
			{
				case TRBType::NO_OP_CMD:
					push_event(evt);
					break;

				case TRBType::ENABLE_SLOT:
				{
					uint8_t allocated_slot = 1;
					if(slots.find(allocated_slot) == slots.end())
					{
						slots[allocated_slot]				   = std::make_shared<xhci_slot>();
						slots[allocated_slot]->attached_device = ports[0];
					}

					evt.control.fields.control = (allocated_slot << 8);
					push_event(evt);
					break;
				}

				case TRBType::DISABLE_SLOT:
				{
					uint8_t slot_id = trb.get_slot_id();
					slots.erase(slot_id);
					push_event(evt);
					break;
				}

				case TRBType::ADDRESS_DEVICE:
				{
					uint8_t slot_id = trb.get_slot_id();
					auto it			= slots.find(slot_id);
					if(it != slots.end() && it->second)
					{
						uint64_t input_ctx_addr		= trb.parameter & ~0xFULL;
						constexpr uint64_t ctx_size = 32;

						// Slot Context goes right after Input Control Context
						uint64_t slot_ctx_addr = input_ctx_addr + ctx_size;
						uint32_t slot_dword1   = 0;
						read_dma(slot_ctx_addr + 4, &slot_dword1, 4);
						uint8_t root_port = (slot_dword1 >> 16) & 0xFF; // Root Hub Port Number

						if(root_port >= 1 && root_port <= XHCI_MAX_PORTS)
							it->second->attached_device = ports[root_port - 1];

						// Endpoint Context 0 (EP0) goes right after Input Control Context + Slot Context
						uint64_t ep0_ctx_addr = input_ctx_addr + 2 * ctx_size;
						uint32_t deq_lo = 0, deq_hi = 0;
						read_dma(ep0_ctx_addr + 8, &deq_lo, 4);	 // DWord2: DCS + TR Dequeue Lo
						read_dma(ep0_ctx_addr + 12, &deq_hi, 4); // DWord3: TR Dequeue Hi

						it->second->ep0_pcs		   = deq_lo & 1;
						it->second->ep0_tr_dequeue = (static_cast<uint64_t>(deq_hi) << 32) | (deq_lo & ~0xFULL);
					}
					push_event(evt);
					break;
				}
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
	void XHCI::push_event(trb_t& evt)
	{
		auto& intr = interrupters[0];
		if(intr.ring_base == 0) return;

		evt.control.fields.cycle = (intr.pcs ? 1 : 0);

		write_dma(intr.event_enqueue_ptr, &evt, sizeof(trb_t));

		intr.event_enqueue_ptr += sizeof(trb_t);

		if(intr.event_enqueue_ptr >= intr.ring_end)
		{
			intr.event_enqueue_ptr = intr.ring_base;
			intr.pcs			   = !intr.pcs;
		}

		intr.iman |= 1;
		sts.fields.eint = 1;
		update_irq();
	}

	void XHCI::write_interrupter_reg(size_t intr_idx, uint32_t reg_offset, uint32_t val)
	{
		auto& intr = interrupters[intr_idx];

		switch(reg_offset)
		{
			case 0x08:						// ERSTSZ
				intr.erstsz = val & 0xFFFF; // lower 16 бит
				break;

			case 0x10: // ERSTBA Low 32 bits
				intr.erstba = (intr.erstba & 0xFFFFFFFF00000000ULL) | val;
				break;

			case 0x14: // ERSTBA High 32 bits
				intr.erstba = (intr.erstba & 0x00000000FFFFFFFFULL) | (static_cast<uint64_t>(val) << 32);
				update_event_ring_segment(intr_idx);
				break;

			case 0x18:															   // ERDP Low 32 bits
				intr.erdp = (intr.erdp & 0xFFFFFFFF00000000ULL) | (val & ~0xFULL); // first 4 bits are flags
				break;

			case 0x1C: // ERDP High 32 bits
				intr.erdp = (intr.erdp & 0x00000000FFFFFFFFULL) | (static_cast<uint64_t>(val) << 32);
				break;
		}
	}

	void XHCI::update_event_ring_segment(size_t intr_idx)
	{
		auto& intr = interrupters[intr_idx];

		if(intr.erstba == 0 || intr.erstsz == 0) return;

		uint64_t table_phys_addr = intr.erstba & ~0x3FULL;

		erst_entry_t seg_entry{};
		read_dma(table_phys_addr, &seg_entry, sizeof(seg_entry));

		intr.ring_base		= seg_entry.ring_segment_base & ~0x3FULL;
		intr.ring_size_trbs = seg_entry.ring_segment_size;
		intr.ring_end		= intr.ring_base + (intr.ring_size_trbs * sizeof(trb_t));

		if(intr.event_enqueue_ptr < intr.ring_base || intr.event_enqueue_ptr >= intr.ring_end)
		{
			intr.event_enqueue_ptr = intr.ring_base;
		}
	}
}
