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
#include "libfdt.h"
#include "memory_map.hpp"
#include <cstdint>

namespace rv64vm::dev
{
	struct Machine;

	/**
	 * @ingroup RV64VM-API
	 * @brief Device base structure
	 */
	struct Device : std::enable_shared_from_this<Device>
	{
		/**
		 * @brief Device constructor
		 * @note You must run all FDT functions here.
		 */
		Device(uint64_t start, uint64_t size, fdt_node* fdt, rv64vm::runner::MemoryMap* mmap)
			: start(start), size(size), end(start + size), mmap(mmap) {

			  };
		/**
		 * @brief MMAP pointer
		 * @note Defined by constructor
		 */
		rv64vm::runner::MemoryMap* mmap;
		/**
		 * @brief Device memory start address
		 * @note Defined by constructor
		 */
		uint64_t start;
		/**
		 * @brief Device memory size
		 * @note Defined by constructor
		 */
		uint64_t size;
		/**
		 * @brief Device memory end address
		 * @note Defined by constructor
		 */
		uint64_t end;

		/**
		 * @brief Device read function
		 */
		virtual uint64_t read(uint64_t addr, MemorySize size) { return 0; }
		/**
		 * @brief Device write function
		 */
		virtual void write(uint64_t addr, MemorySize size, uint64_t val) {}
		/**
		 * @brief Device tick function
		 * @details This function would call almost(optimization) every tick
		 */
		virtual void tick() {}

		/**
		 * @brief Returns device
		 * @warning You would break something if you overwrite this.
		 */
		template <typename T>
		std::shared_ptr<T> get()
		{
			return std::dynamic_pointer_cast<T>(shared_from_this());
		}
	};
}
