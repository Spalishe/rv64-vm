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

#include "../../include/jit/rvjit.hpp"
#include "../../include/hart.hpp"

#include <cstring>
#include <sys/mman.h>

#ifdef USE_JIT

namespace rv64vm::jit
{
	using namespace rv64vm::runner;

	uint8_t* JIT_Context::arena_alloc(size_t nbytes)
	{
		if(cur == nullptr || cur_used + nbytes > JIT_ARENA_BYTES)
		{
			void* p = mmap(nullptr, JIT_ARENA_BYTES, PROT_READ | PROT_WRITE | PROT_EXEC,
						   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			if(p == MAP_FAILED)
				__builtin_trap();
			arenas.push_back((uint8_t*)p);
			cur		 = (uint8_t*)p;
			cur_used = 0;
		}
		uint8_t* p = cur + cur_used;
		cur_used += (nbytes + 15) & ~15ULL;
		return p;
	}

	void JIT_Context::release_arenas()
	{
		for(uint8_t* a : arenas)
			munmap(a, JIT_ARENA_BYTES);
		arenas.clear();
		cur		 = nullptr;
		cur_used = 0;
	}

	JITExec JIT_Context::lookup(uint64_t phys_pc, uint64_t asid)
	{
		CachedBlock& e = cache[index_of(phys_pc)];
		JITExec out;
		if(e.valid && e.start_phys == phys_pc && e.asid == asid && e.smc_epoch == g_smc_epoch.load())
		{
			out.fn	  = e.fn;
			out.count = e.count;
		}
		return out;
	}

	bool JIT_Context::hot_tick(uint64_t phys_pc)
	{
		const size_t i = index_of(phys_pc);
		GiveUpSlot& g  = giveup[i];
		if(g.skip && g.phys == phys_pc)
			return false; // known non-JIT starter: let the interpreter have it
		CachedBlock& e = cache[i];
		return e.hot.fetch_add(1, std::memory_order_relaxed) + 1 >= RVJIT_HOT_THRESHOLD;
	}

	JITExec JIT_Context::compile(Hart& h, uint64_t va_pc, uint64_t phys_pc)
	{
		std::lock_guard<std::mutex> lk(mtx);

		// Already compiled by a concurrent hart?
		JITExec fast = lookup(phys_pc, h.satp.fields.asid);
		if(fast.fn != nullptr)
			return fast;

		// ---- decode the guest stream into straight-line ALU ops ----------
		JIT_Block blk;
		blk.start_phys = phys_pc;
		JIT_Emitter em(&blk);
		em.emit_prologue();

		uint64_t pc_va = va_pc;
		uint32_t count = 0;
		while(count < RVJIT_MAX_INSTRUCTIONS)
		{
			if(em.eof()) // buffer guard; stop early
				break;

			uint64_t phys;
			InstructionCache* cache;
			MemoryReturn mr = h.fetchInstruction(pc_va, phys, cache);
			if(!mr.is_success)
			{
				break; // would trap: let the interpreter take it
			}
			if(cache->pc == 0)
			{
				break; // illegal instruction
			}
			const uint32_t inst = cache->data.inst;
			if((inst & 0x3) != 0x3)
			{
				break; // compressed: interpreter path (no JIT yet)
			}
			if(cache->inst->jit_func == nullptr)
			{
				break; // not JIT-able (control/mem/system/fence)
			}
			// jit_func always emits this guest instruction into the buffer
			// before returning, so it must be counted even when the function
			// reports that the buffer is nearly exhausted (a false return
			// only means "stop the block right after me").
			const bool keep = cache->inst->jit_func(h, const_cast<InstructionData&>(cache->data), blk, em);
			count++;
			pc_va += 4;
			if(!keep)
				break; // compiled, but the block ends after it
		}

		if(count < RVJIT_MIN_INSTRUCTIONS)
		{
			// The block starts on a non-JIT-able instruction; remember that so
			// hot_tick() stops asking us to recompile it on every dispatch.
			GiveUpSlot& g = giveup[index_of(phys_pc)];
			g.phys		  = phys_pc;
			g.skip		  = true;
			return {};
		}

		em.emit_epilogue(count);
		blk.count		= count;
		blk.bytes_guest = count * 4;
		blk.asid		= h.satp.fields.asid;
		blk.smc_epoch	= g_smc_epoch.load();

		// ---- materialize into an executable (writable) arena -------------
		uint8_t* dst = arena_alloc(blk.code.pos);
		if(dst == nullptr)
			return {};
		memcpy(dst, blk.code.bytes, blk.code.pos);

		// Extend the self-modifying-code protection over the block's pages.
		mark_block_executed(phys_pc, blk.bytes_guest);

		CachedBlock& e = cache[index_of(phys_pc)];
		e.fn		   = (JITCompiledFunc)(void*)dst;
		e.start_phys   = phys_pc;
		e.asid		   = h.satp.fields.asid;
		e.smc_epoch	   = g_smc_epoch.load();
		e.count		   = count;
		e.valid		   = true;

		return { e.fn, count };
	}

	void JIT_Context::mark_block_executed(uint64_t phys_pc, uint64_t guest_bytes)
	{
		mark_page_executed(phys_pc);
		mark_page_executed(phys_pc + guest_bytes);
	}

	void JIT_Context::invalidate_all()
	{
		std::lock_guard<std::mutex> lk(mtx);
		for(auto& e : cache)
		{
			e.valid = false;
			e.hot.store(0, std::memory_order_relaxed);
		}
		for(auto& g : giveup)
		{
			g.phys = 0;
			g.skip = false;
		}
		release_arenas();
	}
}

#endif
