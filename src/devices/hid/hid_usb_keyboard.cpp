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

#include "../../../include/devices/hid/hid_usb_keyboard.hpp"
#include "../../../include/devices/plic.hpp"
#include <cstdint>
namespace rv64vm::dev
{
	HID_USB_Keyboard::HID_USB_Keyboard(runner::Machine& cpu) : USBHIDDevice(generate_report_descriptor(HID_USB_Keyboard::report_descriptor_items), 0x1b36, 1)
	{
	}

	void HID_USB_Keyboard::handle_output_report(uint8_t report_id, const std::vector<uint8_t>& data)
	{
		if(data.empty()) return;

		uint8_t leds = data[0];
		on_led_state_change(leds);
	}
	void HID_USB_Keyboard::on_led_state_change(uint8_t leds)
	{
		bool num_lock	 = leds & 0x01;
		bool caps_lock	 = leds & 0x02;
		bool scroll_lock = leds & 0x04;

		(void)num_lock;
		(void)caps_lock;
		(void)scroll_lock;
	}
	void HID_USB_Keyboard::update(uint8_t modifiers, uint8_t key_1, uint8_t key_2, uint8_t key_3, uint8_t key_4, uint8_t key_5, uint8_t key_6, bool rollover)
	{
		if(report_queue.size() >= 64)
		{
			return;
		}

		report[0] = modifiers;
		report[1] = 0x00; // Reserved

		if(rollover)
		{
			// 0x01 = ErrorRollOver
			std::fill(report.begin() + 2, report.end(), 0x01);
		}
		else
		{
			report[2] = key_1;
			report[3] = key_2;
			report[4] = key_3;
			report[5] = key_4;
			report[6] = key_5;
			report[7] = key_6;
		}

		push_report(report);
	}
	bool HID_USB_Keyboard::get_interrupt_report(std::vector<uint8_t>& out)
	{
		out = report;
		return true;
	}
}
