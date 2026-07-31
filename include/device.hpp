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

	struct Device : std::enable_shared_from_this<Device>
	{
		Device(uint64_t start, uint64_t size, fdt_node* fdt, rv64vm::runner::MemoryMap* mmap) : start(start), size(size), end(start + size), mmap(mmap) {

																								};
		rv64vm::runner::MemoryMap* mmap;
		uint64_t start;
		uint64_t size;
		uint64_t end;

		virtual uint64_t read(uint64_t addr, MemorySize size) { return 0; }
		virtual void write(uint64_t addr, MemorySize size, uint64_t val) {}
		virtual void tick() {}

		// Returns device
		template <typename T>
		std::shared_ptr<T> get()
		{
			return std::dynamic_pointer_cast<T>(shared_from_this());
		}
	};
}
