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

#include "defines/traps.hpp"
#include "device.hpp"
#include "memory_map.hpp"
#include <cstdint>
#include <memory>
#include <vector>
namespace rv64vm::runner
{
	class Hart;
	class Machine;

	class MMIO
	{
	  public:
		MMIO(MemoryMap* mmap, uint64_t mem_size);

		uint64_t memsize;
		std::vector<std::shared_ptr<::rv64vm::dev::Device>> devs;

		// Performs write, returns whether write operation was successful
		MemoryReturn write(Hart& h, uint64_t vaddr, MemorySize size, uint64_t val);
		// Preforms read, returns whether read operation was successful
		MemoryReturn read(Hart& h, uint64_t vaddr, MemorySize size, void* val);

		// Creates new device
		template <typename T, typename... Args>
		std::shared_ptr<T> create_device(Args&&... args)
		{
			auto new_device = std::make_shared<T>(std::forward<Args>(args)...);
			devs.push_back(new_device);
			return new_device;
		}
		// Creates new device calling it auto function
		template <typename T>
		std::shared_ptr<T> create_device_auto(Machine& cpu)
		{
			auto new_device = T::init_auto(cpu);
			devs.push_back(new_device);
			return new_device;
		}

		// Runs tick() on every created device
		void tick_all()
		{
			for(const auto& dev : devs)
			{
				if(dev)
				{
					dev->tick();
				}
			}
		}

		// Returns device
		template <typename T>
		std::shared_ptr<T> get()
		{
			for(const auto& dev : devs)
			{
				if(auto sel = std::dynamic_pointer_cast<T>(dev))
				{
					return sel;
				}
			}
			return NULL;
		}

	  private:
		MemoryMap* mmap;
		inline uint64_t read_dram_fast(uint64_t vaddr, MemorySize size);
	};
}
