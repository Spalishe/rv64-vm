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
#ifdef USE_JIT
#include "../../include/rvjit/rvjit.hpp"
#include "../../include/hart.hpp"
#include "../../include/rvjit/rvjit_emit.hpp"
#include "../../include/rvjit/rvjit_x86_64.hpp"
#include <cassert>

namespace rv64vm::jit
{
	void JIT_Context::handleInstruction(rv64vm::runner::Hart& h, rv64vm::runner::InstructionCache& cache, uint64_t prev_pc)
	{
		uint64_t pc = prev_pc;
		if(prev_pc < 0x80000000) return;

		JIT_Function& entry = jits[jit_index(pc)];

		if(entry.valid && entry.pc == pc)
			return;

		auto jc = cache.inst->jit_func;

		if(block_c)
		{
			if(jc == nullptr || block.count >= RVJIT_MAX_INSTRUCTIONS || pc > block.pc + block.size)
			{
				goto end_block_gen;
				return;
			}
			if(!(pc < block.pc + block.size))
			{
				bool stop = jc(h, cache.data, block, emitter);
				block.size += cache.inst->size;
				block.count++;
				if(stop)
					goto end_block_gen;
			}

			if(block.count >= RVJIT_MAX_INSTRUCTIONS || pc > block.pc + block.size)
			{
				goto end_block_gen;
				return;
			}
			return;
		}

		// Check if there any reference of this instruction in decoder
		{
			// If not block creating rn
			if(jc == nullptr) return;

			block_c		   = true;
			// memset(block.bytes, 0, block.byte_pos);
			// memset(&block.inst_addr_jmp, 0xFF, sizeof(block.inst_addr_jmp));
			block.byte_pos = 0;
			block.valid	   = true;
			block.pc	   = pc;
			block.size	   = 0;
			block.count	   = 0;
			block.jmp_labels.clear();
			block.prologue_offs = 0;
			block.outgoing_links.clear();

			emitter.reset();
			emitter.rvjit_emit_prologue(block);

			bool stop	= jc(h, cache.data, block, emitter);
			block.size	= cache.inst->size;
			block.count = 1;
			if(stop)
				goto end_block_gen;
		}
		return;

	end_block_gen:
		block_c = false;
		if(block.count >= RVJIT_MIN_INSTRUCTIONS)
		{
			// Check if our arena is overfilled
			if(arenas[last_arena].used_size + RVJIT_FUNC_SIZE > arenas[last_arena].size)
			{
				// Create new arena
				createNewArena();
			}
			auto& arena = arenas[last_arena];

			emitter.rvjit_emit_epilogue(block);
			emitter.link_out(block, this);

			/*char name[64];
			snprintf(name, 64, "/tmp/jit_0x%lx.bin", block.pc);
			FILEhttps://i.ibb.co/7dCCzgMS/image.png* f = fopen(name, "wb");
			fwrite(block.bytes, 1, block.byte_pos, f);
			fclose(f);
			printf("jit: 0x%lx\n", block.pc);*/

			// We built block sized enough. Go go gadget w^x allocations
			JIT_Function func = arena.push_function(block.bytes, block.byte_pos, last_arena);
			func.inst_size	  = block.size;
			func.pc			  = block.pc;

			const uint64_t block_start = block.pc - 0x80000000ULL;
			const uint64_t block_end   = block_start + block.size - 1;

			const size_t first_page = block_start >> 12;
			const size_t last_page	= block_end >> 12;

			for(size_t page = first_page; page <= last_page; ++page)
				jit_page_bitmap[page] = 1;

			func.page_version  = page_verion_bitmap[(block.pc - 0x80000000) >> 12];
			func.prologue_offs = block.prologue_offs;
			emitter.link_waiting(&func, this);
			uint32_t slot = jit::jit_index(block.pc);
			jits[slot]	  = std::move(func);
			arenas[last_arena].function_slots.push_back(slot);
			count++;
		}
	}
	void JIT_Context::stopBlock()
	{
		if(block_c)
		{
			block_c = false;
		}
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
