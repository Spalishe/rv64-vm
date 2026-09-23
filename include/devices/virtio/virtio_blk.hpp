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
#include "../../libfdt.h"
#include "../plic.hpp"

#include "../../fwd.hpp"
#include "virtio_mmio.hpp"
#include <cstdint>
#include <cstring>
#include <string>

namespace rv64vm::dev
{
	class VirtIO_BLK : public Device
	{
	  public:
		VirtIO_BLK(uint64_t base, uint64_t size, runner::Machine& cpu, fdt_node* fdt, FILE* image);

		static std::shared_ptr<VirtIO_BLK> init_auto(runner::Machine& cpu);

	  private:
		uint64_t read(uint64_t addr, MemorySize size);
		void write(uint64_t addr, MemorySize size, uint64_t val);
		void copy_from_dram(uint64_t gpa, void* dst, uint64_t len);
		void copy_to_dram(uint64_t gpa, const void* src, uint64_t len);

		// queue/descriptor processing
		void process_queue(uint32_t qsel);
		bool fetch_descriptor_chain(uint16_t head, std::vector<VirtqDesc>& out_chain, const VirtQueueState& q);

		// disk image operations
		bool disk_read(uint64_t sector, void* buf, size_t bytes);
		bool disk_write(uint64_t sector, const void* buf, size_t bytes);

		// utilities
		void raise_irq();

	  private:
		PLIC* plic;
		uint8_t irq_num;
		std::string image_path;
		FILE* disk = nullptr; // image file

		// device state
		uint64_t device_features	 = 0;
		uint64_t driver_features	 = 0;
		uint32_t device_features_sel = 0;
		uint32_t driver_features_sel = 0;
		uint32_t device_status		 = 0;
		uint32_t interrupt_status	 = 0;

		uint64_t config_space[128];

		uint32_t queue_sel = 0;
		VirtQueueState queue0;

		uint32_t config_generation = 0;
		uint64_t capacity_sectors  = 0; // number of 512-byte sectors
	};
}
