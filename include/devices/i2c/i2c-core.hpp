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
#include "i2c-slave.hpp"
#include <cstdint>
#include <memory>
#include <vector>

#define I2C_REG_PRER_LO 0x0
#define I2C_REG_PRER_HI 0x1 << 2
#define I2C_REG_CTR		0x2 << 2
#define I2C_REG_TXR_RXR 0x3 << 2
#define I2C_REG_CR_SR	0x4 << 2

#define I2C_CTR_EN_BIT	0x7
#define I2C_CTR_IEN_BIT 0x6

union i2c_cr
{
	struct [[gnu::packed]]
	{
		uint8_t IACK : 1;
		uint8_t : 2;
		uint8_t ACK : 1;
		uint8_t WR : 1;
		uint8_t RD : 1;
		uint8_t STO : 1;
		uint8_t STA : 1;
	} fields;

	uint8_t raw;
};
union i2c_sr
{
	struct [[gnu::packed]]
	{
		uint8_t IF : 1;
		uint8_t TIP : 1;
		uint8_t : 3;
		uint8_t Al : 1;
		uint8_t BUSY : 1;
		uint8_t RxACK : 1;
	} fields;

	uint8_t raw;
};

class PLIC;
struct I2CSlave;
class I2C : public Device
{
  public:
	I2C(uint64_t base, Machine& cpu, fdt_node* fdt);
	static std::shared_ptr<I2C> init_auto(Machine& cpu);

	std::vector<std::shared_ptr<I2CSlave>> slaves;

	template <typename T, typename... Args>
	std::shared_ptr<I2CSlave> create_device(Args&&... args)
	{
		auto new_device = std::make_shared<T>(std::forward<Args>(args)...);
		slaves.push_back(new_device);
		return new_device;
	}

  private:
	Machine& cpu;
	PLIC* plic;
	int irq_num;

	uint8_t prer_lo = 0xFF;
	uint8_t prer_hi = 0xFF;
	uint8_t ctr		= 0x00; // control register
	uint8_t txr		= 0x00; // transmit register
	uint8_t rxr		= 0x00; // receive register
	i2c_cr cr;				// command register
	i2c_sr sr;				// status register

	bool device_selected				  = false;
	uint16_t current_address			  = 0xFFFF;
	std::shared_ptr<I2CSlave> current_dev = 0;
	bool is_read_operation				  = false;
	bool int_line						  = false;

	void raise_interrupt();
	void lower_interrupt();
	void handle_device_write(uint8_t data);
	uint8_t handle_device_read();
	void execute_command();
	void tick();

	bool op_active = false;

	uint64_t read(uint64_t addr, MemorySize size);
	void write(uint64_t addr, MemorySize size, uint64_t val);
	const int _run_delay = 100;
	int _delay			 = 0;
};
