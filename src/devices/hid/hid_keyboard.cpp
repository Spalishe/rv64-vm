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

#include "../../../include/devices/hid/hid_keyboard.hpp"
#include "../../../include/devices/plic.hpp"
#include <cstdint>
namespace rv64vm::dev
{
	HID_Keyboard::HID_Keyboard(runner::Machine& cpu, fdt_node* fdt) : HIDOverI2C(cpu, fdt, generate_report_descriptor(HID_Keyboard::report_descriptor_items), 10)
	{
		hid_descriptor[0xa] = (input_report_size & 0xFF);		 // 0x0A
		hid_descriptor[0xb] = ((input_report_size >> 8) & 0xFF); // 0x00
	}

	void HID_Keyboard::hid_event_output_report_write()
	{
	}
	void HID_Keyboard::hid_event_data_register_write()
	{
	}
	void HID_Keyboard::hid_event_command_register_write()
	{
		uint8_t arg	   = command_register[0];
		uint8_t opcode = command_register[1] & 0x0F;

		switch(opcode)
		{
			case 0x01: // RESET
			{
				io_offset = 0;

				input_report[0x00] = 0x00;
				input_report[0x01] = 0x00;
				std::queue<std::vector<uint8_t>> empty;
				report_queue.swap(empty);
				plic->set_pending(irq_num, true);
				break;
			}
			case 0x08: // SET_POWER
				if(arg == 0x01)
				{
					printf("Device power set to ON\n");
					// is_powered_on = true;
				}
				else if(arg == 0x00)
				{
					// is_powered_on = false;
				}

				break;
		}
	}

	void HID_Keyboard::update(uint8_t modifiers, uint8_t key_1, uint8_t key_2, uint8_t key_3, uint8_t key_4, uint8_t key_5, uint8_t key_6, bool rollover)
	{
		if(report_queue.size() >= 64)
		{
			return;
		}
		if(rollover)
			report_queue.push({ modifiers, 0x00, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1 });
		else
			report_queue.push({ modifiers, 0x0, key_1, key_2, key_3, key_4, key_5, key_6 });

		if(report_queue.size() == 1)
		{
			hid_input_report_write(report_queue.front());
		}
		if(!is_transmitting)
		{
			plic->set_pending(irq_num, true);
		}
	}
}
