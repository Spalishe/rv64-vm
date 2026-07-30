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

#include "../../include/devices/plic.hpp"
#include "../../include/machine.hpp"
namespace rv64vm::dev
{
#ifndef PLIC_CTX_THRESHOLD
#define PLIC_CTX_THRESHOLD 0x0
#endif
#ifndef PLIC_CTX_CLAIMCOMP
#define PLIC_CTX_CLAIMCOMP 0x4
#endif

	PLIC::PLIC(uint64_t start, uint64_t size, runner::Machine& cpu, uint32_t max_sources, fdt_node* fdt)
		: Device(start, size, fdt, cpu.get_mmap()),
		  num_harts(cpu.get_hart_count()),
		  num_contexts(cpu.get_hart_count() * 2),
		  max_sources(max_sources),
		  cpu(cpu)
	{
		cpu.get_mmap()->add_region(start, size);

		pending_words_count = (max_sources + 31) / 32;
		enable_words_count	= pending_words_count;

		priorities.resize(max_sources + 1, 0);
		pending_words.resize(pending_words_count, 0);

		contexts.resize(num_contexts);
		for(uint32_t i = 0; i < num_contexts; ++i)
		{
			contexts[i].enables.resize(enable_words_count, 0);
			contexts[i].threshold	   = 0;
			contexts[i].interrupt_line = false;

			contexts[i].hart_id			= static_cast<int>(i / 2);
			contexts[i].supervisor_mode = (i % 2) == 1;
		}

		if(fdt)
		{
			struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
			std::vector<uint32_t> irq_ext;

			for(uint32_t i = 0; i < cpu.get_hart_count(); ++i)
			{
				struct fdt_node* cpu_node = fdt_node_find_reg(cpus, "cpu", i);
				if(!cpu_node) continue;

				struct fdt_node* cpu_irq = fdt_node_find(cpu_node, "interrupt-controller");
				if(!cpu_irq) continue;

				uint32_t irq_phandle = fdt_node_get_phandle(cpu_irq);
				if(!irq_phandle) continue;
				fdt_node_free(cpu_irq);
				fdt_node_free(cpu_node);

				// M-mode external interrupt
				irq_ext.push_back(irq_phandle);
				irq_ext.push_back(MIE_MEIE_BIT);

				// S-mode external interrupt
				irq_ext.push_back(irq_phandle);
				irq_ext.push_back(MIE_SEIE_BIT);
			}
			fdt_node_free(cpus);

			struct fdt_node* plic_fdt = fdt_node_create_reg("plic", start);
			fdt_node_add_prop_reg(plic_fdt, "reg", start, size);
			fdt_node_add_prop_str(plic_fdt, "compatible", "sifive,plic-1.0.0");
			fdt_node_add_prop(plic_fdt, "interrupt-controller", nullptr, 0);
			fdt_node_add_prop_u32(plic_fdt, "#interrupt-cells", 1);
			fdt_node_add_prop_u32(plic_fdt, "#address-cells", 0);
			fdt_node_add_prop_u32(plic_fdt, "riscv,ndev", max_sources);

			if(!irq_ext.empty())
			{
				fdt_node_add_prop_cells(plic_fdt, "interrupts-extended", irq_ext, irq_ext.size());
			}

			fdt_node* soc = fdt_node_find(fdt, "soc");
			fdt_node_add_child(soc, plic_fdt);
			fdt_node_free(soc);
		}
	}

	std::shared_ptr<PLIC> PLIC::init_auto(runner::Machine& cpu)
	{
		return std::make_shared<PLIC>(0x0C000000, 0x400000, cpu, 64, cpu.get_fdt());
	}

	int PLIC::acquire_irq()
	{
		uint32_t val = last_irq_registered;
		last_irq_registered++;
		return val;
	}

	int PLIC::last_irq()
	{
		return last_irq_registered - 1;
	}

	void PLIC::set_pending(uint32_t source_id, bool pending)
	{
		if(source_id == 0 || source_id > max_sources)
			return;

		uint32_t word_idx = source_id / 32;
		uint32_t bit_mask = 1U << (source_id % 32);

		if(pending)
		{
			pending_words[word_idx] |= bit_mask;
		}
		else
			pending_words[word_idx] &= ~bit_mask;

		update_all_contexts();
	}

	void PLIC::tick()
	{
		// update_all_contexts();
	}

	uint64_t PLIC::read(uint64_t addr, MemorySize size)
	{
		if(size != MemorySize::Int) return 0;

		uint64_t offset = addr - start;

		// Priority registers
		if(offset < 0x1000)
		{
			uint32_t idx = static_cast<uint32_t>(offset / 4);
			if(idx >= 1 && idx <= max_sources)
				return priorities[idx];
			return 0;
		}

		// Pending (read-only)
		if(offset >= 0x1000 && offset < 0x1080)
		{
			uint32_t word_idx = static_cast<uint32_t>((offset - 0x1000) / 4);
			if(word_idx < pending_words_count)
				return pending_words[word_idx];
			return 0;
		}

		// Enable registers
		if(offset >= 0x2000 && offset < 0x200000)
		{
			uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x2000) / 0x80);
			if(ctx_idx >= num_contexts) return 0;

			uint32_t word_offset = static_cast<uint32_t>(((offset - 0x2000) % 0x80) / 4);
			if(word_offset < enable_words_count)
				return contexts[ctx_idx].enables[word_offset];
			return 0;
		}

		// Threshold and Claim
		[[likely]] if(offset >= 0x200000 && offset < 0x200000 + num_contexts * 0x1000)
		{
			uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x200000) / 0x1000);
			if(ctx_idx >= num_contexts) return 0;

			uint32_t sub_offset = static_cast<uint32_t>((offset - 0x200000) % 0x1000);
			if(sub_offset == 0)
			{
				return contexts[ctx_idx].threshold;
			}
			else if(sub_offset == 4)
			{
				return claim_interrupt(ctx_idx);
			}
		}
		return 0;
	}

	void PLIC::write(uint64_t addr, MemorySize size, uint64_t value)
	{
		if(size != MemorySize::Int) return;

		uint64_t offset = addr - start;

		// Priority
		if(offset < 0x1000)
		{
			uint32_t idx = static_cast<uint32_t>(offset / 4);
			if(idx >= 1 && idx <= max_sources)
			{
				priorities[idx] = static_cast<uint32_t>(value & 0x7FFFFFFF);
				update_all_contexts();
			}
			return;
		}

		// Pending is read-only
		if(offset >= 0x1000 && offset < 0x1080) return;

		// Enable
		if(offset >= 0x2000 && offset < 0x200000)
		{
			uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x2000) / 0x80);
			if(ctx_idx >= num_contexts) return;

			uint32_t word_offset = static_cast<uint32_t>(((offset - 0x2000) % 0x80) / 4);
			if(word_offset < enable_words_count)
			{
				contexts[ctx_idx].enables[word_offset] = static_cast<uint32_t>(value);
				update_context(ctx_idx);
			}
			return;
		}

		// Threshold and Complete
		[[likely]] if(offset >= 0x200000 && offset < 0x200000 + num_contexts * 0x1000)
		{
			uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x200000) / 0x1000);
			if(ctx_idx >= num_contexts) return;

			uint32_t sub_offset = static_cast<uint32_t>((offset - 0x200000) % 0x1000);
			if(sub_offset == 0)
			{
				contexts[ctx_idx].threshold = static_cast<uint32_t>(value & 0x7FFFFFFF);
				update_context(ctx_idx);
			}
			else if(sub_offset == 4)
			{
				complete_interrupt(ctx_idx, static_cast<uint32_t>(value));
			}
			return;
		}
	}

	uint32_t PLIC::find_highest_pending_enabled(uint32_t ctx_idx)
	{
		const Context& ctx	   = contexts[ctx_idx];
		uint32_t best_id	   = 0;
		uint32_t best_priority = 0;

		uint32_t word_count = (max_sources + 31) / 32;
		for(uint32_t word = 0; word < word_count; ++word)
		{
			uint32_t active = pending_words[word] & ctx.enables[word];

			while(active)
			{
				uint32_t bit = std::countr_zero(active);
				uint32_t id	 = word * 32 + bit;

				if(id != 0 && id <= max_sources)
				{
					uint32_t prio = priorities[id];

					if(prio > ctx.threshold)
					{
						if(prio > best_priority || (prio == best_priority && id < best_id))
						{
							best_priority = prio;
							best_id		  = id;
						}
					}
				}

				active &= active - 1;
			}
		}
		return best_id;
	}

	uint32_t PLIC::claim_interrupt(uint32_t ctx_idx)
	{
		uint32_t id = find_highest_pending_enabled(ctx_idx);
		if(id != 0)
		{
			uint32_t word = id / 32;
			uint32_t bit  = id % 32;
			pending_words[word] &= ~(1U << bit);
			update_context(ctx_idx);
		}
		return id;
	}

	void PLIC::complete_interrupt(uint32_t ctx_idx, uint32_t id)
	{
		if(id == 0 || id > max_sources) return;
		uint32_t word = id / 32;
		uint32_t bit  = id % 32;
		pending_words[word] &= ~(1U << bit);
		update_context(ctx_idx);
	}

	void PLIC::update_context(uint32_t ctx_idx)
	{
		Context& ctx	  = contexts[ctx_idx];
		bool should_raise = (find_highest_pending_enabled(ctx_idx) != 0);
		if(should_raise != ctx.interrupt_line)
		{
			ctx.interrupt_line = should_raise;
			signal_cpu_interrupt(ctx.hart_id, ctx.supervisor_mode, should_raise);
		}
	}

	void PLIC::update_all_contexts()
	{
		for(uint32_t i = 0; i < num_contexts; ++i)
			update_context(i);
	}

	void PLIC::signal_cpu_interrupt(int hart_id, bool supervisor_mode, bool level)
	{
		if(hart_id < 0 || hart_id >= 64)
			return;

		runner::Hart& hart = cpu.get_hart(hart_id);

		const uint64_t MEIP_BIT = 1ULL << 11;
		const uint64_t SEIP_BIT = 1ULL << 9;

		if(supervisor_mode)
		{
			if(level)
				hart.ip.fields.SEIP = 1;
			else
				hart.ip.fields.SEIP = 0;
		}
		else
		{
			if(level)
				hart.ip.fields.MEIP = 1;
			else
				hart.ip.fields.MEIP = 0;
		}
	}
}
