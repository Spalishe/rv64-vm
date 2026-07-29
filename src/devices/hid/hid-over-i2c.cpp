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

#include "../../../include/devices/hid/hid-over-i2c.hpp"
#include "../../../include/devices/plic.hpp"
#include "../../../include/machine.hpp"

HIDOverI2C::HIDOverI2C(Machine& cpu, fdt_node* fdt, std::vector<uint8_t> report_desc, uint16_t input_report_size) : I2CSlave(cpu.get_mmio()->get<PLIC>().get()->acquire_irq(), HID_I2C_BUFFER_SIZE),
																													plic(cpu.get_mmio()->get<PLIC>().get()), irq_num(plic->last_irq()),
																													report_desc(report_desc), input_report_size(input_report_size),
																													cpu(cpu)
{
	hid_descriptor[0x4] = (report_desc.size() & 0xFF);
	hid_descriptor[0x5] = ((report_desc.size() >> 8) & 0xFF);

	input_report	 = new uint8_t[input_report_size];
	output_report	 = new uint8_t[1024];
	data_register	 = new uint8_t[1024];
	command_register = new uint8_t[1024];

	if(fdt != NULL)
	{
		struct fdt_node* i2c_fdt = fdt_node_create_reg("i2c", irq_num);
		fdt_node_add_prop_u32(i2c_fdt, "reg", irq_num);
		fdt_node_add_prop_str(i2c_fdt, "compatible", "hid-over-i2c");
		fdt_node* soc  = fdt_node_find(fdt, "soc");
		fdt_node* plic = fdt_node_find_reg(soc, "plic", 0x0C000000);
		fdt_node_add_prop_u32(i2c_fdt, "interrupt-parent", fdt_node_get_phandle(plic));
		fdt_node_free(plic);
		fdt_node_add_prop_u32(i2c_fdt, "interrupts", irq_num);
		fdt_node_add_prop_u32(i2c_fdt, "hid-descr-addr", 0x01);
		fdt_node_add_prop(i2c_fdt, "wakeup-source", NULL, 0);

		fdt_node* i2c = fdt_node_find_reg_any(soc, "i2c");
		fdt_node_add_child(i2c, i2c_fdt);
		fdt_node_free(soc);
		fdt_node_free(i2c);
	}
};

void HIDOverI2C::stop_transmit()
{
	is_transmitting = false;
	cur_reg			= 0x03;
	switch(last_event)
	{
		case(WriteEvent::COMMAND):
			hid_event_command_register_write();
			break;
		case(WriteEvent::DATA):
			hid_event_data_register_write();
			break;
		case(WriteEvent::OUTPUT):
			hid_event_output_report_write();
			break;
		default:
			break;
	}
	if(report_queue.size() > 0)
		plic->set_pending(irq_num, true);
};
void HIDOverI2C::start_transmit()
{
	io_offset		= 0;
	is_transmitting = true;
	last_event		= WriteEvent::NONE;
};

uint8_t HIDOverI2C::dev_read(bool m_ack)
{
	uint8_t val = 0;
	if(cur_reg == 0x01)
	{
		if(io_offset < 30)
		{
			val = hid_descriptor[io_offset];
		}
	}
	else if(cur_reg == 0x02)
	{
		// Report descriptor
		if(io_offset < report_desc.size())
		{
			val = report_desc[io_offset];
		}
	}
	else if(cur_reg == 0x03)
	{
		// Input Report Register
		if(io_offset < input_report_size) val = input_report[io_offset];
		if(io_offset >= (input_report_size - 1))
		{
			if(!report_queue.empty())
				report_queue.pop();

			if(report_queue.size() > 0)
			{
				hid_input_report_write(report_queue.front());
				plic->set_pending(irq_num, true);

				return val;
			}
			else
				plic->set_pending(irq_num, false);
		}
	}
	else if(cur_reg == 0x05)
	{
		// Command Register
		if(io_offset < 1024) val = command_register[io_offset];
	}
	else if(cur_reg == 0x06)
	{
		// Data Register
		if(io_offset < 1024) val = data_register[io_offset];
	}

	io_offset++;
	return val;
}
void HIDOverI2C::dev_write(uint8_t val)
{
	if(io_offset == 0)
	{
		cur_reg = val; // LSB
		io_offset++;
	}
	else if(io_offset == 1)
	{
		cur_reg |= ((uint16_t)val << 8); // MSB
		io_offset++;
	}
	else
	{
		if(cur_reg == 0x5)
		{
			// Command register
			if((io_offset - 2) < 1024) command_register[io_offset - 2] = val;
			io_offset++;
			last_event = WriteEvent::COMMAND;
			return;
		}
		else if(cur_reg == 0x6)
		{
			// Data register
			if((io_offset - 2) < 1024) data_register[io_offset - 2] = val;
			io_offset++;
			last_event = WriteEvent::DATA;
			return;
		}
		else if(cur_reg == 0x07)
		{
			// Output Report Register
			if((io_offset - 2) < 1024) output_report[io_offset - 2] = val;
			io_offset++;
			last_event = WriteEvent::OUTPUT;
			return;
		}
	}
}

void HIDOverI2C::hid_input_report_write(const std::vector<uint8_t>& bytes)
{
	uint16_t total_len = bytes.size() + 2;
	input_report[0x00] = total_len & 0xFF;
	input_report[0x01] = (total_len >> 8) & 0xFF;
	for(int i = 0x0; i < bytes.size(); i++)
	{
		input_report[i + 2] = bytes[i];
	}
}

void HIDOverI2C::hid_data_register_write(const std::vector<uint8_t>& bytes)
{
	uint16_t total_len	= bytes.size() + 2;
	data_register[0x00] = total_len & 0xFF;
	data_register[0x01] = (total_len >> 8) & 0xFF;
	for(int i = 0x0; i < bytes.size(); i++)
	{
		data_register[i + 2] = bytes[i];
	}
}
