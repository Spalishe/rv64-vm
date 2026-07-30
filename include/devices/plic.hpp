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
#include <cstdint>
#include <vector>

#include "../fwd.hpp"

namespace rv64vm::dev
{
	class PLIC : public Device
	{
	  public:
		PLIC(uint64_t start, uint64_t size, runner::Machine& cpu, uint32_t max_sources, fdt_node* fdt);

		void set_pending(uint32_t source_id, bool pending);

		uint64_t read(uint64_t addr, MemorySize size);
		void write(uint64_t addr, MemorySize size, uint64_t val);
		void tick();
		static std::shared_ptr<PLIC> init_auto(runner::Machine& cpu);
		int acquire_irq();
		int last_irq();

	  private:
		struct Context
		{
			uint32_t threshold = 0;
			std::vector<uint32_t> enables;
			int hart_id			 = -1;
			bool supervisor_mode = false;
			bool interrupt_line	 = false;
		};

		uint32_t max_sources;
		uint32_t num_harts;
		uint32_t num_contexts;
		uint32_t pending_words_count;
		uint32_t enable_words_count;
		runner::Machine& cpu;
		uint32_t last_irq_registered = 1;

		std::vector<uint32_t> priorities; // indexes 0..max_sources, 0 not used
		std::vector<uint32_t> pending_words;
		std::vector<Context> contexts;

		uint32_t find_highest_pending_enabled(uint32_t ctx_idx);

		// Claim: returns irq ID and resets it pending bit
		uint32_t claim_interrupt(uint32_t ctx_idx);

		// Complete: resets pending bit to complete irq
		void complete_interrupt(uint32_t ctx_idx, uint32_t id);

		void update_context(uint32_t ctx_idx);

		void update_all_contexts();

		void signal_cpu_interrupt(int hart_id, bool supervisor_mode, bool level);
	};
}
