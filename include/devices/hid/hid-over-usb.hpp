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
#include "../usb/usb-dev.hpp"
#include <cstdint>
#include <cstring>
#include <queue>
#include <vector>

namespace rv64vm::dev
{
	// USB Request Types (bmRequestType)
	enum class USBRequestType : uint8_t
	{
		STANDARD_GET_DESCRIPTOR = 0x06,
		STANDARD_SET_CONFIG		= 0x09,
		HID_GET_REPORT			= 0x01,
		HID_SET_IDLE			= 0x0A,
		HID_SET_PROTOCOL		= 0x0B
	};

	class USBHIDDevice : public USBDevice
	{
	  protected:
		std::vector<uint8_t> report_desc;
		std::queue<std::vector<uint8_t>> report_queue;
		uint8_t idle_rate{ 0 };
		uint8_t protocol{ 1 }; // 1 = Report Protocol, 0 = Boot Protocol

		// Standard USB Descriptors
		std::vector<uint8_t> device_desc;
		std::vector<uint8_t> config_desc;

		void build_descriptors(uint16_t vendor_id, uint16_t product_id);
		virtual void handle_output_report(uint8_t report_id, const std::vector<uint8_t>& data) {}

	  public:
		USBHIDDevice(std::vector<uint8_t> report_descriptor, uint16_t vid = 0x1234, uint16_t pid = 0x5678);
		virtual ~USBHIDDevice() = default;

		// Request handle on EP0 (Control)
		std::vector<uint8_t> handle_control_request(const usb_setup_packet_t& setup);

		// Data handle in EP1 IN (Interrupt)
		bool get_next_input_report(std::vector<uint8_t>& out_report);

		bool get_interrupt_report(std::vector<uint8_t>& data) override
		{
			// Idle keyboards only report on key events; deliver an empty
			// boot report to the host during enumeration/probe instead of NAK.
			if(!get_next_input_report(data))
			{
				data.assign(8, 0);
			}
			return true;
		}

		// Add message from device to report.
		void push_report(const std::vector<uint8_t>& report)
		{
			report_queue.push(report);
		}

		// Receive data from host to device
		void handle_control_data_out(const usb_setup_packet_t& setup, const std::vector<uint8_t>& data_out)
		{
			uint8_t req_type = setup.bmRequestType;
			uint8_t req		 = setup.bRequest;

			// Class-Specific Request (0x20)
			if((req_type & 0x60) == 0x20)
			{
				if(req == 0x09) // SET_REPORT
				{
					uint8_t report_type = setup.wValue >> 8; // 0x01 = Input, 0x02 = Output, 0x03 = Feature
					uint8_t report_id	= setup.wValue & 0xFF;

					if(report_type == 0x02) // Output Report
					{
						handle_output_report(report_id, data_out);
					}
				}
			}
		}
	};
}
