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
#include "defs.hpp"
#include <cstdint>
#include <vector>

struct USBDevice : std::enable_shared_from_this<USBDevice>
{
  public:
	virtual ~USBDevice() = default;

	// IN
	virtual std::vector<uint8_t> handle_control_request(const usb_setup_packet_t& pkt)
	{
		uint8_t type = pkt.bRequest;

		// GET_DESCRIPTOR
		if(type == 0x06)
		{
			uint8_t desc_type = pkt.wValue >> 8;
			if(desc_type == 1) return get_device_descriptor(); // Device Desc
			if(desc_type == 2) return get_config_descriptor(); // Config Desc
		}

		return {}; // STALL
	}

	// OUT
	virtual void handle_control_data_out(const usb_setup_packet_t& pkt, const std::vector<uint8_t>& data)
	{
		// Serial
		if(pkt.bRequest == 0x20) // SET_LINE_CODING
		{
			uint32_t baud_rate = *reinterpret_cast<const uint32_t*>(data.data());
			set_baud_rate(baud_rate);
		}
	}

  private:
	std::vector<uint8_t> get_device_descriptor() { return {}; }
	std::vector<uint8_t> get_config_descriptor() { return {}; }
	virtual void set_baud_rate(uint32_t baud) {}
};
