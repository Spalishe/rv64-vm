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
#include "../../defines/traps.hpp"
#include "../../libfdt.h"
#include "../../memory_map.hpp"
#include <cstdint>
#include <cstring>
#include <memory>

namespace rv64vm::dev
{
	struct BAR
	{
		uint64_t address = 0;
		uint64_t size	 = 0;
		uint32_t flags	 = 0;
	};

	struct PCI_Device : std::enable_shared_from_this<PCI_Device>
	{
		PCI_Device()
		{
			std::fill(std::begin(config), std::end(config), 0);
			std::fill(std::begin(bars), std::end(bars), BAR{ 0, 0, 0 });
			std::fill(std::begin(bar_masks), std::end(bar_masks), 0);
		}
		PCI_Device(uint16_t vendor_id, uint16_t device_id, uint8_t class_code)
		{
			std::fill(std::begin(config), std::end(config), 0);
			std::fill(std::begin(bars), std::end(bars), BAR{ 0, 0, 0 });

			write_config_fast<uint16_t>(0x00, vendor_id); // Vendor ID
			write_config_fast<uint16_t>(0x02, device_id); // Device ID
			write_config_fast<uint8_t>(0x0B, class_code); // Class Code

			std::fill(std::begin(bar_masks), std::end(bar_masks), 0);
		}

		virtual ~PCI_Device() = default;

		// Default PCI Configuration Space (256 byte)
		uint8_t config[256];
		BAR bars[6];
		uint32_t bar_masks[6]; // Size masks for BAR0-BAR5

		void init_bar(int idx, uint64_t bar_size, uint32_t flags = 0x02)
		{
			bars[idx].size	= bar_size;
			bars[idx].flags = flags;
			// if size will 4КБ (0x1000), mask will be ~(0x1000 - 1) = 0xFFFFF000
			bar_masks[idx]	= ~(bar_size - 1);
		}

		// ECAM R/W
		virtual uint64_t read_config(uint64_t offset, MemorySize size)
		{
			if(offset + (int)size > 256) return 0xFFFFFFFF;

			uint64_t val = 0;
			std::memcpy(&val, &config[offset], (int)size);
			return val;
		}

		virtual void write_config(uint64_t offset, MemorySize size, uint64_t val)
		{
			if(offset + (int)size > 256) return;

			if(offset >= 0x10 && offset <= 0x24)
			{
				int bar_idx = (offset - 0x10) / 4;
				if(bar_idx >= 0 && bar_idx < 6 && bars[bar_idx].size > 0)
				{
					uint32_t write_val = (uint32_t)val;

					if(write_val == 0xFFFFFFFF)
					{
						// Return size
						uint32_t response = bar_masks[bar_idx] | bars[bar_idx].flags;
						std::memcpy(&config[offset], &response, 4);
					}
					else
					{
						// Set new address
						bars[bar_idx].address = write_val & bar_masks[bar_idx];
						std::memcpy(&config[offset], &write_val, 4);
					}
					return;
				}
			}

			std::memcpy(&config[offset], &val, (int)size);
		}

		virtual uint64_t read_mmio(uint64_t local_offset, MemorySize size) { return 0; }
		virtual void write_mmio(uint64_t local_offset, MemorySize size, uint64_t val) {}

		virtual void tick() {}

		template <typename T>
		std::shared_ptr<T> get()
		{
			return std::dynamic_pointer_cast<T>(shared_from_this());
		}

		template <typename T>
		void write_config_fast(uint64_t offset, T val)
		{
			std::memcpy(&config[offset], &val, sizeof(T));
		}
	};
};
