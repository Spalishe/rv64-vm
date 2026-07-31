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
#include "../../../include/devices/i2c/i2c-core.hpp"
#include "../../../include/devices/plic.hpp"
#include "../../../include/libfdt.h"
#include "../../../include/machine.hpp"

namespace rv64vm::dev
{
	I2C::I2C(uint64_t start, runner::Machine& cpu, fdt_node* fdt)
		: Device(start, 0x20, fdt, cpu.get_mmap()),
		  plic(cpu.get_mmio()->get<PLIC>().get()), irq_num(plic->acquire_irq()),
		  cpu(cpu)
	{
		sr.raw = 0x0;
		cr.raw = 0x0;

		cpu.get_mmap()->add_region(start, size);
		if(fdt != NULL)
		{
			struct fdt_node* i2c_fdt = fdt_node_create_reg("i2c", start);
			fdt_node_add_prop_reg(i2c_fdt, "reg", start, size);
			fdt_node_add_prop_str(i2c_fdt, "compatible", "opencores,i2c-ocores");
			fdt_node* soc  = fdt_node_find(fdt, "soc");
			fdt_node* plic = fdt_node_find_reg(soc, "plic", 0x0C000000);
			fdt_node_add_prop_u32(i2c_fdt, "interrupt-parent", fdt_node_get_phandle(plic));
			fdt_node_free(plic);
			fdt_node_add_prop_u32(i2c_fdt, "interrupts", irq_num);
			fdt_node_add_prop_u32(i2c_fdt, "#address-cells", 1);
			fdt_node_add_prop_u32(i2c_fdt, "#size-cells", 0);
			fdt_node_add_prop_u32(i2c_fdt, "reg-shift", 2);
			fdt_node_add_prop_u32(i2c_fdt, "reg-io-width", 1);
			fdt_node_add_prop_u32(i2c_fdt, "opencores,ip-clock-frequency", 0x1312d00);

			fdt_node_add_prop_str(i2c_fdt, "status", "okay");

			fdt_node_add_child(soc, i2c_fdt);
			fdt_node_free(soc);
		}
	}

	std::shared_ptr<I2C> I2C::init_auto(runner::Machine& cpu)
	{
		return std::make_shared<I2C>(0x10030000, cpu, cpu.get_fdt());
	}

	uint64_t I2C::read(uint64_t addr, MemorySize size)
	{
		uint64_t offs = addr - start;

		switch(offs)
		{
			case I2C_REG_PRER_LO:
				return prer_lo;
			case I2C_REG_PRER_HI:
				return prer_hi;
			case I2C_REG_CTR:
				return ctr;
			case I2C_REG_TXR_RXR:
				return rxr;
			case I2C_REG_CR_SR:
				return sr.raw;
			default:
				return 0;
		}
	}

	void I2C::tick()
	{
		if(!op_active) return;

		_delay++;
		if(_delay < _run_delay) return;
		execute_command();

		op_active = false;
	}

	void I2C::execute_command()
	{
		// reset int
		if(cr.fields.IACK)
		{
			sr.fields.IF = 0;
			lower_interrupt();
			cr.fields.IACK = 0;
		}

		if(!cr.fields.STA && !cr.fields.STO && !cr.fields.RD && !cr.fields.WR)
		{
			return;
		}

		bool transaction_started = false;

		// set dev addr
		if(cr.fields.STA)
		{
			sr.fields.BUSY	= 1;
			current_address = 0xFFFF;
		}
		// write
		if(cr.fields.WR)
		{
			transaction_started = true;
			if(current_address == 0xFFFF)
			{
				current_address	  = txr >> 1;
				is_read_operation = txr & 1;

				device_selected = false;

				for(auto dev : slaves)
				{
					if(dev->address == current_address)
					{
						if(!device_selected)
						{
							current_dev = dev;
							dev->start_transmit();
						}
						device_selected = true;
						sr.fields.RxACK = 0; // ACK
						break;
					}
				}
			}
			else
			{
				sr.fields.TIP	= 1;
				sr.fields.RxACK = 1;
				if(device_selected && current_dev)
				{
					handle_device_write(txr);
					sr.fields.RxACK = 0;
				}
			}
		}
		// read
		if(cr.fields.RD)
		{
			transaction_started = true;
			sr.fields.TIP		= 1;
			if(device_selected && current_dev)
			{
				rxr				= handle_device_read();
				sr.fields.RxACK = 0;
			}
			else
			{
				rxr				= 0xFF;
				sr.fields.RxACK = 1;
			}
		}

		// stop
		if(cr.fields.STO)
		{
			if(device_selected && current_dev)
			{
				current_dev->stop_transmit();
			}
			device_selected		= false;
			transaction_started = true;
			sr.fields.BUSY		= 0; // release bus
		}

		if(transaction_started)
		{
			sr.fields.TIP = 0; // transfer complete
			sr.fields.IF  = 1; // irq flag of transaction complete
			raise_interrupt();
		}
		fflush(stdout);
	}

	void I2C::raise_interrupt()
	{
		if((ctr & 0x40) && !int_line)
		{
			// IEN set
			int_line = true;
			plic->set_pending(irq_num, true);
		}
	}

	void I2C::lower_interrupt()
	{
		if(int_line)
		{
			int_line = false;
			plic->set_pending(irq_num, false);
		}
	}

	void I2C::handle_device_write(uint8_t data)
	{
		if(device_selected)
		{
			current_dev->dev_write(data);
		}
	}
	uint8_t I2C::handle_device_read()
	{
		if(device_selected)
		{
			return current_dev->dev_read(cr.fields.ACK);
		}
		return 0;
	}

	void I2C::write(uint64_t addr, MemorySize size, uint64_t val)
	{
		uint64_t offs = addr - start;
		switch(offs)
		{
			case I2C_REG_PRER_LO:
				prer_lo = val;
				break;
			case I2C_REG_PRER_HI:
				prer_hi = val;
				break;
			case I2C_REG_CTR:
				ctr = val;
				if(!(ctr & (1 << 7)))
				{
					sr.raw			= 0;
					device_selected = false;
					current_address = 0xFFFF;
					lower_interrupt();
				}
				break;
			case I2C_REG_TXR_RXR:
				txr = val;
				break;
			case I2C_REG_CR_SR:
			{
				if(!(ctr & (1 << 7))) return;
				cr.raw	  = val;
				op_active = true;
				_delay	  = 0;

				sr.fields.TIP = 1;
				sr.fields.IF  = 0;
				break;
			}
			default:
				return;
		}
		return;
	}
}
