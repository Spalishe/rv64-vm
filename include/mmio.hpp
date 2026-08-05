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

	/**
	 * @ingroup RV64VM-API
	 * @brief RV64-VM Memory-Mapped Input/Output controller.
	 * @details This class implements RISC-V basic MMIO structure which holds all devices
	 */
	class MMIO
	{
	  public:
		/**
		 * @brief MMIO constructor
		 * @details Creates MMIO object
		 */
		MMIO(MemoryMap* mmap, uint64_t mem_size);
		/**
		 * @brief MMIO destructor
		 * @details Removes MMIO object
		 */
		~MMIO() {};

		/**
		 * @brief Device list
		 * @details Contains list of all created and using devices in system.
		 */
		std::vector<std::shared_ptr<::rv64vm::dev::Device>> devs;

		/**
		 * @brief Write operation
		 * @details Writes data to DRAM. If defined address is beyond DRAM base address then it check for all devices and writes data to them.
		 */
		MemoryReturn write(Hart& h, uint64_t vaddr, MemorySize size, uint64_t val);
		/**
		 * @brief Read operation
		 * @details Reads data from DRAM. If defined address is beyond DRAM base address then it check for all devices and reads their memory.
		 */
		MemoryReturn read(Hart& h, uint64_t vaddr, MemorySize size, void* val);

		/**
		 * @brief Creates new device
		 * @details Creates new T device and automatically adds it to device list.
		 */
		template <typename T, typename... Args>
		std::shared_ptr<T> create_device(Args&&... args)
		{
			auto new_device = std::make_shared<T>(std::forward<Args>(args)...);
			devs.push_back(new_device);
			return new_device;
		}
		/**
		 * @brief Creates new device automatically
		 * @details Creates new T device by calling it auto create function.
		 * @note It is recommended to use this function to create devices.
		 */
		template <typename T>
		std::shared_ptr<T> create_device_auto(Machine& cpu)
		{
			auto new_device = T::init_auto(cpu);
			devs.push_back(new_device);
			return new_device;
		}

		/**
		 * @brief Devices tick function
		 * @details Wrapper that automatically will call every registered device tick function
		 * @note This function automatically calls in Machine, no need to call it manually **unless you have a reason**.
		 */
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

		/**
		 * @brief Device getter function
		 * @details Returns first-found T from device list
		 */
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
		uint64_t memsize;
		MemoryMap* mmap;
		inline uint64_t read_dram_fast(uint64_t vaddr, MemorySize size);
	};
}
