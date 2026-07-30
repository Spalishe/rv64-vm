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
#include "../device.hpp"

#include "../fwd.hpp"

namespace rv64vm::dev
{
	class SYSCON : public Device
	{
	  public:
		SYSCON(uint64_t base, uint64_t size, runner::Machine& cpu, fdt_node* fdt);
		static std::shared_ptr<SYSCON> init_auto(runner::Machine& cpu);

	  private:
		runner::Machine& cpu;
		uint64_t read(uint64_t addr, MemorySize size);
		void write(uint64_t addr, MemorySize size, uint64_t val);
	};
}
