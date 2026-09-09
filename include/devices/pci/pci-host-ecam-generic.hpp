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
#include "pci-device.hpp"
#include <cstdint>
#include <memory>

namespace rv64vm::dev
{
	struct PLIC;
	class PCI_HEG : public Device
	{
	  public:
		PCI_HEG(uint64_t base, runner::Machine& cpu, fdt_node* fdt);
		static std::shared_ptr<PCI_HEG> init_auto(runner::Machine& cpu);

		void attach_device(uint8_t slot, PCI_Device* dev);

	  private:
		::rv64vm::runner::Machine& cpu;
		PLIC* plic;
		int irq_num;

		uint64_t read(uint64_t addr, MemorySize size);
		void write(uint64_t addr, MemorySize size, uint64_t val);

		std::array<PCI_Device*, 32> devices;
	};
}
