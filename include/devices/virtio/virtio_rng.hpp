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
	class VirtIO_RNG : public Device
	{
	  public:
		VirtIO_RNG(uint64_t base, uint64_t size, runner::Machine& cpu, fdt_node* fdt);

		static std::shared_ptr<VirtIO_RNG> init_auto(runner::Machine& cpu);

	  private:
		uint64_t read(uint64_t addr, MemorySize size);
		void write(uint64_t addr, MemorySize size, uint64_t val);
		void copy_from_dram(uint64_t gpa, void* dst, uint64_t len);
		void copy_to_dram(uint64_t gpa, const void* src, uint64_t len);

		// queue/descriptor processing
		void process_queue(uint32_t qsel);
		bool fetch_descriptor_chain(uint16_t head, std::vector<VirtqDesc>& out_chain, const VirtQueueState& q);

		// utilities
		void raise_irq();

	  private:
		PLIC* plic;
		uint8_t irq_num;
		std::string image_path;

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

		typedef struct
		{
			uint64_t state;
			uint64_t inc; // stream
		} virtio_pcg32_ctx_t;

		virtio_pcg32_ctx_t rng_ctx;

		void virtio_rng_seed(uint64_t init_state, uint64_t init_seq)
		{
			rng_ctx.state = 0U;
			rng_ctx.inc	  = (init_seq << 1u) | 1u;
			rng_ctx.state = rng_ctx.state * 6364136223846793005ULL + rng_ctx.inc;
			rng_ctx.state += init_state;
			rng_ctx.state = rng_ctx.state * 6364136223846793005ULL + rng_ctx.inc;
		}

		inline uint32_t virtio_rng_next32()
		{
			uint64_t oldstate	= rng_ctx.state;
			// LCG
			rng_ctx.state		= oldstate * 6364136223846793005ULL + rng_ctx.inc;
			// XSH RR
			uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
			uint32_t rot		= oldstate >> 59u;
			return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
		}

		void virtio_rng_fill_buffer(uint8_t* buf, size_t len)
		{
			size_t i = 0;

			while(i + 4 <= len)
			{
				uint32_t rand_val = virtio_rng_next32();
				buf[i]			  = (uint8_t)(rand_val);
				buf[i + 1]		  = (uint8_t)(rand_val >> 8);
				buf[i + 2]		  = (uint8_t)(rand_val >> 16);
				buf[i + 3]		  = (uint8_t)(rand_val >> 24);
				i += 4;
			}

			if(i < len)
			{
				uint32_t rand_val = virtio_rng_next32();
				while(i < len)
				{
					buf[i] = (uint8_t)rand_val;
					rand_val >>= 8;
					i++;
				}
			}
		}
	};
}
