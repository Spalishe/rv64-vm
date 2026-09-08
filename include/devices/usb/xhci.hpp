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
#include "../../device.hpp"
#include "../../fwd.hpp"
#include <cstdint>
#include <memory>

namespace rv64vm::dev
{
	static constexpr uint8_t XHCI_CAPLENGTH	  = 0x20;
	static constexpr uint16_t XHCI_HCIVERSION = 0x0110; // version 1.1.0
	static constexpr uint8_t XHCI_MAX_PORTS	  = 6;		// Port count
	static constexpr uint32_t XHCI_RTSOFF	  = 0x100;	// offs Runtime regs
	static constexpr uint32_t XHCI_DBOFF	  = 0x200;	// offs Doorbell regs

	union xhci_usbcmd
	{
		struct
		{
			uint32_t runstop : 1; // Run/Stop
			uint32_t hcrst : 1;	  // Reset
			uint32_t inte : 1;	  // Global interrupt enable
			uint32_t hsee : 1;	  // Host System Error Enable
			uint32_t : 3;
			uint32_t lhcrst : 1; // Light Host Controller Reset
			uint32_t css : 1;	 // Controller Save State
			uint32_t crs : 1;	 // Controller Restore State
			uint32_t ewe : 1;	 // Enable Wrap Event
			uint32_t eu3s : 1;	 // Enable U3 MFINDEX Stop
			uint32_t : 20;
		} fields;
		uint32_t raw;
	};

	union xhci_usbsts
	{
		struct
		{
			uint32_t hchalted : 1; // Controller halted
			uint32_t hse : 1;	   // Host System Error
			uint32_t eint : 1;	   // Event Interrupt (new unhandled events)
			uint32_t pcd : 1;	   // Port Change Detect
			uint32_t : 7;
			uint32_t cnr : 1; // Controller Not Ready (1 = no init)
			uint32_t hce : 1; // Host Controller Error
			uint32_t : 19;
		} fields;
		uint32_t raw;
	};

	union xhci_portsc
	{
		struct
		{
			uint32_t ccs : 1; // Current Connect Status (connected)
			uint32_t ped : 1; // Port Enabled/Disabled
			uint32_t : 1;
			uint32_t oca : 1;	// Over-Current Active
			uint32_t pr : 1;	// Port Reset (Хост инициирует сброс)
			uint32_t pls : 4;	// Port Link State (U0, U1, U2, U3, etc)
			uint32_t pp : 1;	// Port Power (Питание порта)
			uint32_t speed : 4; // Port Speed (1=Full, 2=Low, 3=High, 4=Super)
			uint32_t pic : 2;	// Port Indicator Control
			uint32_t lws : 1;	// Port Link State Write Strobe
			uint32_t csc : 1;	// Connect Status Change (RW1C)
			uint32_t pec : 1;	// Port Enabled/Disabled Change (RW1C)
			uint32_t wrc : 1;	// Warm Port Reset Change (RW1C)
			uint32_t occ : 1;	// Over-Current Change (RW1C)
			uint32_t prc : 1;	// Port Reset Change (RW1C)
			uint32_t plc : 1;	// Port Link State Change (RW1C)
			uint32_t cec : 1;	// Port Config Error Change (RW1C)
			uint32_t cas : 1;	// Cold Attach Status
			uint32_t wce : 1;	// Wake on Connect Enable
			uint32_t wde : 1;	// Wake on Disconnect Enable
			uint32_t woe : 1;	// Wake on Over-Current Enable
			uint32_t : 2;
			uint32_t dr : 1;  // Device Removable
			uint32_t wpr : 1; // Warm Port Reset
		} fields;
		uint32_t raw;
	};

	// Interrupter Register Set
	struct xhci_interrupter
	{
		uint32_t iman;	 // Interrupter Management (Allow ints)
		uint32_t imod;	 // Interrupter Moderation
		uint32_t erstsz; // Event Ring Segment Table Size
		uint32_t res;
		uint64_t erstba; // Event Ring Segment Table Base Address
		uint64_t erdp;	 // Event Ring Dequeue Pointer
	};

	enum class TRBType : uint8_t
	{
		NORMAL			= 1,
		SETUP_STAGE		= 2,
		DATA_STAGE		= 3,
		STATUS_STAGE	= 4,
		ISOCH			= 5,
		LINK			= 6,
		EVENT_DATA		= 7,
		NO_OP			= 8,
		ENABLE_SLOT		= 9,
		DISABLE_SLOT	= 10,
		ADDRESS_DEVICE	= 11,
		CONFIG_ENDPOINT = 12,
		EVAL_CONTEXT	= 13,
		RESET_ENDPOINT	= 14,
		STOP_ENDPOINT	= 15,
		SET_TR_DEQUEUE	= 16,
		RESET_DEVICE	= 17,
		NO_OP_CMD		= 23,

		// Events
		TRANSFER_EVENT			 = 32,
		CMD_COMPLETION_EVENT	 = 33,
		PORT_STATUS_CHANGE_EVENT = 34
	};

	// TRB Completion Codes (xHCI spec 6.4.5)
	enum class TRBCompletionCode : uint8_t
	{
		INVALID				  = 0,
		SUCCESS				  = 1,
		DATA_BUFFER_ERROR	  = 2,
		BABBLE_DETECTED		  = 3,
		USB_TRANSACTION_ERROR = 4,
		TRB_ERROR			  = 5,
		STALL_ERROR			  = 6,
		RESOURCE_ERROR		  = 7,
		BANDWIDTH_ERROR		  = 8,
		NO_SLOTS_AVAILABLE	  = 9,
		INVALID_STREAM_TYPE	  = 10,
		SLOT_NOT_ENABLED	  = 11,
		ENDPOINT_NOT_ENABLED  = 12,
		SHORT_PACKET		  = 13,
		RING_UNDERRUN		  = 14,
		RING_OVERRUN		  = 15,
		PARAMETER_ERROR		  = 17,
		CONTEXT_STATE_ERROR	  = 19,
		EVENT_RING_FULL		  = 21,
		COMMAND_RING_STOPPED  = 24,
		COMMAND_ABORTED		  = 25,
		STOPPED				  = 26
	};

	// Generic 16-byte TRB layout
	struct alignas(16) trb_t
	{
		uint64_t parameter;
		uint32_t status;
		union
		{
			struct
			{
				uint32_t cycle : 1;
				uint32_t ent : 1;	   // Evaluate Next TRB / Toggle Cycle
				uint32_t flags : 8;	   // ISP, FIFO, CH, IOC, etc.
				uint32_t type : 6;	   // TRBType
				uint32_t control : 16; // Slot ID, VF ID, Endpoint ID, or TRB sub-fields
			} fields;
			uint32_t raw;
		} control;

		TRBType get_type() const
		{
			return static_cast<TRBType>(control.fields.type);
		}

		uint8_t get_slot_id() const
		{
			return static_cast<uint8_t>((control.fields.control >> 8) & 0xFF);
		}

		uint8_t get_endpoint_id() const
		{
			return static_cast<uint8_t>(control.fields.control & 0x1F);
		}
	};

	// Event Ring Segment Table Entry (ERSTE)
	struct erst_entry_t
	{
		uint64_t ring_segment_base;
		uint32_t ring_segment_size;
		uint32_t reserved;
	};

	struct PLIC;
	class XHCI : public Device
	{
	  public:
		XHCI(uint64_t base, runner::Machine& cpu, fdt_node* fdt);
		static std::shared_ptr<XHCI> init_auto(runner::Machine& cpu);

	  private:
		::rv64vm::runner::Machine& cpu;
		PLIC* plic;
		int irq_num;

		uint64_t crcr_dequeue{ 0 };
		bool pcs{ true }; // Producer Cycle State for Command Ring
		bool event_pcs{ true };

		void process_command_ring();
		void push_event(const trb_t& evt);
		trb_t read_trb(uint64_t addr);
		void write_trb(uint64_t addr, const trb_t& trb);

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
		uint64_t read(uint64_t addr, MemorySize size);
		void write(uint64_t addr, MemorySize size, uint64_t val);
	};
}
