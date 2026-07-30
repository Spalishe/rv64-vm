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
#include "../i2c/i2c-core.hpp"
#include "../i2c/i2c-slave.hpp"
#include <cstdint>
#include <queue>
#include <vector>

namespace rv64vm::dev
{
	static constexpr uint16_t HID_I2C_BUFFER_SIZE = 0x1000;

	class PLIC;
	struct HIDOverI2C : I2CSlave
	{
		HIDOverI2C(runner::Machine& cpu, fdt_node* fdt, std::vector<uint8_t> report_desc, uint16_t input_report_size);
		~HIDOverI2C()
		{
			if(input_report != nullptr) delete[] input_report;
			if(output_report != nullptr) delete[] output_report;
			if(data_register != nullptr) delete[] data_register;
			if(command_register != nullptr) delete[] command_register;
		}

		PLIC* plic;
		int irq_num;
		runner::Machine& cpu;

		std::vector<uint8_t> report_desc;
		uint16_t cur_reg;
		uint16_t io_offset = 0;
		uint16_t input_report_size;

		void dev_write(uint8_t val);
		uint8_t dev_read(bool m_ack);
		void stop_transmit();
		void start_transmit();

		// Input report
		std::queue<std::vector<uint8_t>> report_queue;
		uint8_t* input_report;
		void hid_input_report_write(const std::vector<uint8_t>& bytes);

		// Output report
		uint8_t* output_report;
		virtual void hid_event_output_report_write() {};

		// Data register
		uint8_t* data_register;
		void hid_data_register_write(const std::vector<uint8_t>& bytes);
		virtual void hid_event_data_register_write() {};

		// Command register
		uint8_t* command_register;
		virtual void hid_event_command_register_write() {};

		uint8_t hid_descriptor[30] = {
			0x1e, 0x00,			   // wHIDDescLength (30 байт)
			0x00, 0x01,			   // bcdVersion (v1.00)
			0x64, 0x00,			   // wReportDescLength
			0x02, 0x00,			   // wReportDescRegister (0x0002)
			0x03, 0x00,			   // wInputRegister (0x0003)
			0x00, 0x04,			   // wInputLen
			0x07, 0x00,			   // wOutputRegister
			0x00, 0x04,			   // wOutputLen
			0x05, 0x00,			   // wCommandRegister (0x0005)
			0x06, 0x00,			   // wDataRegister
			0x34, 0x12,			   // wVendorID (0x1234)
			0x78, 0x56,			   // wProductID (0x5678)
			0x01, 0x00,			   // wVersionID
			0x00, 0x00, 0x00, 0x00 // Reserved
		};

		enum class WriteEvent
		{
			NONE	= 0,
			COMMAND = 1,
			DATA	= 2,
			OUTPUT	= 3
		};
		WriteEvent last_event = WriteEvent::NONE;
	};
}
