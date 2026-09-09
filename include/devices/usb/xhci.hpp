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

#pragma once
#include "../../fwd.hpp"
#include "../pci/pci-device.hpp"
#include "defs.hpp"
#include "usb-dev.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace rv64vm::dev
{
	struct PLIC;
	class XHCI : public PCI_Device
	{
	  public:
		XHCI(runner::Machine& cpu);
		void attach_device(uint8_t port_idx, std::shared_ptr<USBDevice> dev, uint8_t port_speed = 3)
		{
			if(port_idx >= XHCI_MAX_PORTS) return;

			ports[port_idx] = dev;

			port_sc[port_idx].fields.ccs   = 1;
			port_sc[port_idx].fields.ped   = 0;
			port_sc[port_idx].fields.pp	   = 1;
			port_sc[port_idx].fields.speed = port_speed;
		}

	  private:
		::rv64vm::runner::Machine& cpu;
		PLIC* plic;
		int irq_num;

		bool event_ring_pcs{ true };
		uint64_t crcr_dequeue{ 0 };
		bool pcs{ true }; // Producer Cycle State for Command Ring
		bool event_pcs{ true };

		xhci_usbcmd cmd;
		xhci_usbsts sts;
		uint32_t config_reg;

		uint64_t dcbaap;	// Device Context Base Address Array Pointer
		uint64_t crcr_base; // Command Ring Control Register

		// ports and interrupts
		xhci_portsc port_sc[XHCI_MAX_PORTS];
		xhci_interrupter interrupters[1];

		void reset();
		void update_irq();
		uint64_t read_mmio(uint64_t addr, MemorySize size);
		void write_mmio(uint64_t addr, MemorySize size, uint64_t val);

		void process_ep0_transfer_ring(uint32_t slot_id, uint64_t trb_dma_addr);
		void process_command_ring();
		void push_event(trb_t& evt);
		trb_t read_trb(uint64_t addr);
		void write_trb(uint64_t addr, const trb_t& trb);
		void read_dma(uint64_t addr, void* dest, size_t size);
		void write_dma(uint64_t addr, const void* src, size_t size);

		void write_doorbell(uint32_t offset, uint32_t val);
		uint64_t get_ep_ctx_tr_enqueue_pointer(uint32_t slot_id, uint32_t ep_index);
		void send_transfer_event(uint32_t slot_id, uint32_t ep_index, TRBCompletionCode code);
		void write_interrupter_reg(size_t intr_idx, uint32_t reg_offset, uint32_t val);
		void update_event_ring_segment(size_t intr_idx);

		std::array<std::shared_ptr<USBDevice>, XHCI_MAX_PORTS> ports;

		// Slot id -> Dev
		std::unordered_map<uint8_t, std::shared_ptr<xhci_slot>> slots;
	};
}
