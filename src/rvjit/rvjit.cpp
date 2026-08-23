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
#include <sys/types.h>
#ifdef USE_JIT
#include "../../include/hart.hpp"
#include "../../include/rvjit/rvjit.hpp"
#include "../../include/rvjit/rvjit_emit.hpp"
#include "../../include/rvjit/rvjit_x86_64.hpp"
#include <cassert>

namespace rv64vm::jit
{
	bool JIT_Context::compileBlock(Hart& h, uint64_t start_va)
	{
		uint64_t phys_pc		= 0;
		InstructionCache* cache = nullptr;

		if(!h.fetchInstruction(start_va, phys_pc, cache).is_success)
			return false;

		auto jc = cache->inst->jit_func;
		if(jc == nullptr)
			return false;

		const uint64_t page = (phys_pc - 0x80000000ULL) >> 12;

		block.byte_pos = 0;
		block.valid	   = true;
		block.va_pc	   = start_va;
		block.pc	   = phys_pc;
		block.size	   = 0;
		block.count	   = 0;
		block.jmp_labels.clear();
		block.prologue_offs = 0;
		block.outgoing_links.clear();

		emitter.reset();
		emitter.rvjit_emit_prologue(block);

		uint64_t va = start_va;

		for(;;)
		{
			bool stop = jc(h, cache->data, block, emitter);
			block.size += cache->inst->size;
			block.count++;

			if(stop) break;
			if(block.count >= RVJIT_MAX_INSTRUCTIONS) break;

			va += cache->inst->size;

			uint64_t next_phys			 = 0;
			InstructionCache* next_cache = nullptr;

			if(!h.fetchInstruction(va, next_phys, next_cache).is_success)
				break;

			const uint64_t next_page = (next_phys - 0x80000000ULL) >> 12;
			if(next_page != page)
				break;

			auto next_jc = next_cache->inst->jit_func;
			if(next_jc == nullptr)
				break;

			cache = next_cache;
			jc	  = next_jc;
		}

		if(block.count < RVJIT_MIN_INSTRUCTIONS)
		{
			block.valid = false;
			return false;
		}

		finalizeBlock(page, h.satp.fields.asid);
		return true;
	}
	void JIT_Context::finalizeBlock(uint64_t page, uint64_t asid)
	{
		if(arenas[last_arena].used_size + RVJIT_FUNC_SIZE > arenas[last_arena].size)
			createNewArena();

		auto& arena = arenas[last_arena];

		emitter.rvjit_emit_epilogue(block);
		emitter.link_out(block, this);

		JIT_Function func = arena.push_function(block.bytes, block.byte_pos, last_arena);
		func.inst_size	  = block.size;
		func.pc			  = block.pc;
		func.asid		  = asid;

		jit_page_bitmap[page]			  = 1;
		jit_page_bitmap_asid[page].valid  = 1;
		jit_page_bitmap_asid[page].asid	  = asid;
		jit_page_bitmap_asid[page].global = asid == UINT64_MAX;

		func.page_version  = page_verion_bitmap[page];
		func.prologue_offs = block.prologue_offs;

		emitter.link_waiting(&func, this);

		uint32_t slot = jit::jit_index(block.pc);
		jits[slot]	  = std::move(func);
		arenas[last_arena].function_slots.push_back(slot);
		count++;
	}

	void JIT_Function::cleanup(JIT_Context* ctx)
	{
		if(!valid)
			return;

		const size_t page_size = ctx->page_size;

		uintptr_t pages[32];
		size_t page_count = 0;

		for(const auto& link : linked)
		{
			JIT_Function& src = ctx->jits[jit::jit_index(link.func_pc)];

			if(!src.valid || src.pc != link.func_pc)
				continue;

			uintptr_t page = (reinterpret_cast<uintptr_t>(src.func) + link.patch_offs)
							 & ~(page_size - 1);

			bool found = false;

			for(size_t i = 0; i < page_count; ++i)
			{
				if(pages[i] == page)
				{
					found = true;
					break;
				}
			}

			if(!found)
			{
				if(page_count < std::size(pages))
					pages[page_count++] = page;
				else
				{
					// fallback, если links неожиданно много
					mprotect(
						reinterpret_cast<void*>(page),
						page_size,
						PROT_READ | PROT_WRITE);
				}
			}
		}

		for(size_t i = 0; i < page_count; ++i)
		{
			mprotect(
				reinterpret_cast<void*>(pages[i]),
				page_size,
				PROT_READ | PROT_WRITE);
		}

		for(const auto& link : linked)
		{
			JIT_Function& src = ctx->jits[jit::jit_index(link.func_pc)];

			if(!src.valid || src.pc != link.func_pc)
				continue;

			JITFunction_cleanup_link(
				reinterpret_cast<uint8_t*>(src.func),
				link.patch_offs);
		}

		for(size_t i = 0; i < page_count; ++i)
		{
			mprotect(
				reinterpret_cast<void*>(pages[i]),
				page_size,
				PROT_READ | PROT_EXEC);
		}

		linked.clear();

		pc			  = 0;
		page_version  = 0;
		inst_size	  = 0;
		prologue_offs = 0;
		valid		  = false;
	}

#include <sys/mman.h>
#include <unistd.h>

	void JIT_Context::createNewArena()
	{
		const size_t arena_size = RVJIT_ARENA_PAGES * sysconf(_SC_PAGESIZE);

		while(total_allocated + arena_size > max_cache_size && !arenas.empty())
		{
			const uint64_t old_idx = arena_order.front();
			arena_order.pop();

			auto arena_it = arenas.find(old_idx);

			if(arena_it != arenas.end())
			{
				JIT_Arena& arena = arena_it->second;

				for(uint32_t slot : arena.function_slots)
				{
					JIT_Function& fn = jits[slot];

					if(fn.valid && fn.arena_index == old_idx)
					{
						fn.cleanup(this);
					}
				}

				total_allocated -= arena.size;
				arenas.erase(arena_it);
			}
		}

		++last_arena;

		auto [it, inserted] = arenas.try_emplace(last_arena);

		JIT_Arena& arena = it->second;
		arena.init();

		total_allocated += arena.size;
		arena_order.push(last_arena);
	}
	void JIT_Arena::allocate()
	{
		_page_size = sysconf(_SC_PAGESIZE);
		size	   = RVJIT_ARENA_PAGES * _page_size;

		void* buffer = mmap(
			nullptr,
			size,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1,
			0);

		if(buffer == MAP_FAILED)
		{
			fprintf(stderr, "[RVJIT] Failed to allocate RW region.\n");
			return;
		}

		base	  = buffer;
		valid	  = true;
		used_size = 0;

		function_slots.clear();
		function_slots.reserve(size / RVJIT_FUNC_SIZE);
	}
	JIT_Function JIT_Arena::push_function(const void* code, size_t code_size, uint64_t arena_index)
	{
		if(used_size + RVJIT_FUNC_SIZE > size)
		{
			fprintf(stderr, "[RVJIT] Arena is full.\n");
			return JIT_Function{};
		}

		if(code_size > RVJIT_FUNC_SIZE)
		{
			fprintf(stderr, "[RVJIT] Emitted code is larger than RVJIT_FUNC_SIZE.\n");
			return JIT_Function{};
		}

		uint8_t* func_pos = static_cast<uint8_t*>(base) + used_size;

		uintptr_t page_addr	 = reinterpret_cast<uintptr_t>(func_pos);
		uintptr_t page_start = page_addr - (page_addr % _page_size);
		// Change permission to READ | WRITE
		if(mprotect(reinterpret_cast<void*>(page_start), _page_size, PROT_READ | PROT_WRITE) == -1)
		{
			fprintf(stderr, "[RVJIT] Failed to change region permission to RW.\n");
			return JIT_Function{};
		}

		// Write bytecode
		std::memcpy(func_pos, code, code_size);
		// Memcpy goes firstly to CPU I-cache, rather than straight to a memory
		__builtin___clear_cache(func_pos, func_pos + code_size);

		// Change permissions back to READ | EXEC
		if(mprotect(reinterpret_cast<void*>(page_start), _page_size, PROT_READ | PROT_EXEC) == -1)
		{
			fprintf(stderr, "[RVJIT] Failed to change region permission to RX.\n");
			return JIT_Function{};
		}

		JIT_Function result;
		result.func		   = reinterpret_cast<JITCompilatedFunc>(func_pos);
		result.size		   = code_size;
		result.valid	   = true;
		result.arena_index = arena_index;
		used_size += RVJIT_FUNC_SIZE;
		return result;
	}
}
#endif
