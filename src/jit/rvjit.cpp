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
#include "../../include/self_mod.hpp"

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
			if(arenas.size() * JIT_ARENA_BYTES >= RVJIT_MAX_CACHE_BYTES)
			{
				g_smc_epoch.fetch_add(1, std::memory_order_release);
				release_arenas();
			}
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
		g_arena_gen.fetch_add(1, std::memory_order_release);
		for(uint8_t* a : arenas)
			munmap(a, JIT_ARENA_BYTES);
		arenas.clear();
		cur		 = nullptr;
		cur_used = 0;
	}

	// One shared chain dispatcher per process: every block exit that cannot hop
	// through its private slot lands here instead of ret-ing into C++. It
	// resolves the exit pc through the direct-mapped hctx jump cache and either
	// jumps into the successor's post-prologue entry or pops the single C++
	// frame.  R12 (ctx) is preserved here; pooled/scratch regs are dead.
	uint8_t* ensure_chain_dispatcher()
	{
		static uint8_t* dispatch = nullptr;
		if(dispatch != nullptr)
			return dispatch;

		void* p = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
					   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if(p == MAP_FAILED)
			__builtin_trap();
		dispatch = (uint8_t*)p;

		x86::CodeBuf cb;

		// entry = &ctx->chain_cache[chain_cache_index(exit_pc, mode) * 4];
		// must stay in sync with chain_cache_index() in rvjit_ctx.hpp.
		x86::mov_mr(cb, x86::REG_RCX, x86::REG_CTX, x86::CTX_OFF_EXIT);
		x86::mov_rr(cb, x86::REG_RAX, x86::REG_RCX);
		x86::shift_r64_imm(cb, 5, x86::REG_RAX, 1); // rax = exit_pc >> 1
		x86::mov_imm64(cb, x86::REG_R10, 0x9E3779B97F4A7C15ULL);
		x86::imul_rr(cb, x86::REG_RAX, x86::REG_R10);
		x86::shift_r64_imm(cb, 5, x86::REG_RAX, 64 - USE_JCHAIN);
		x86::mov_mr(cb, x86::REG_R10, x86::REG_CTX, x86::CTX_OFF_CHAIN_MODE);
		x86::mov_imm64(cb, x86::REG_RDX, 0xD1B54A32D192ED03ULL);
		x86::imul_rr(cb, x86::REG_R10, x86::REG_RDX);
		x86::shift_r64_imm(cb, 5, x86::REG_R10, 64 - USE_JCHAIN);
		x86::xor_rr(cb, x86::REG_RAX, x86::REG_R10);
		x86::and_imm(cb, x86::REG_RAX, x86::CHAIN_CACHE_MASK);
		x86::shift_r64_imm(cb, 4, x86::REG_RAX, 5); // rax = idx * 32 (CHAIN_CACHE_STRIDE)
		x86::add_rr(cb, x86::REG_RAX, x86::REG_CTX);
		x86::add_imm(cb, x86::REG_RAX, x86::CTX_OFF_CHAIN_CACHE);

		// chain_fn != 0, pc == exit_pc, epoch == ctx.chain_epoch,
		// mode == ctx.chain_mode
		x86::mov_mr(cb, x86::REG_R11, x86::REG_RAX, 0);
		x86::test_rr(cb, x86::REG_R11, x86::REG_R11);
		const uint32_t j_nofn = x86::jcc32(cb, 0x4); // je
		x86::cmp_r64_m64(cb, x86::REG_RCX, x86::REG_RAX, 8);
		const uint32_t j_pc = x86::jcc32(cb, 0x5); // jne
		x86::mov_mr(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_CHAIN_EPOCH);
		x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_RAX, 16);
		const uint32_t j_epoch = x86::jcc32(cb, 0x5); // jne
		x86::mov_mr(cb, x86::REG_R10, x86::REG_CTX, x86::CTX_OFF_CHAIN_MODE);
		x86::cmp_r64_m64(cb, x86::REG_R10, x86::REG_RAX, 24);
		const uint32_t j_mode = x86::jcc32(cb, 0x5); // jne

		// Hit: re-stamp the calling block's private exit slot (r15 = its base,
		// set by the exit tail) so later traversals hop directly without the
		// dispatcher.  The epoch/mode guards already ran, so re-stamping is
		// exactly as strong as the four comparisons it replaces.
		x86::mov_rm(cb, x86::REG_R15, 16, x86::REG_RDX);
		x86::mov_rm(cb, x86::REG_R15, 0, x86::REG_R11);
		x86::mov_rm(cb, x86::REG_R15, 8, x86::REG_RCX);
		x86::mov_rm(cb, x86::REG_R15, 24, x86::REG_R10);

		// Hit: set the target block's base pc (exits are computed as
		// entry_pc + delta_va) and jump in right after its prologue
		x86::mov_rm(cb, x86::REG_CTX, x86::CTX_OFF_ENTRY, x86::REG_RCX);
		x86::jmp_r(cb, x86::REG_R11);

		// Miss: pop the single C++ frame and return to the runner, which
		// re-dispatches (the exit tail already accounted its own budget).
		const uint32_t tail = cb.pos;
		x86::pop_r(cb, x86::REG_R12);
		x86::pop_r(cb, x86::REG_R13);
		x86::pop_r(cb, x86::REG_R15);
		x86::pop_r(cb, x86::REG_RBP);
		x86::ret(cb);

		for(uint32_t j : { j_nofn, j_pc, j_epoch, j_mode })
			x86::patch_rel32(cb, j, tail);

		memcpy(p, cb.bytes, cb.pos);
		if(x86::chain_dispatcher() == 0)
			x86::chain_dispatcher() = (uint64_t)dispatch;
		return dispatch;
	}

	// Key: (start_phys, epoch, eff_mode, mxr, sum) - deliberately asid/VA
	// free; see the lookup() declaration for the invariants that make this
	// sound.
	JITExec JIT_Context::lookup(uint64_t phys_pc, uint8_t eff_mode, bool mxr, bool sum)
	{
		const size_t base	 = index_of(phys_pc) * CACHE_WAYS;
		const uint64_t epoch = g_smc_epoch.load(std::memory_order_relaxed);
		for(size_t w = 0; w < CACHE_WAYS; w++)
		{
			CachedBlock& e = cache[base + w];
			if(!e.valid || e.start_phys != phys_pc)
				continue;
			if(e.smc_epoch != epoch)
				continue;
			if(e.eff_mode != eff_mode || e.mxr != mxr || e.sum != sum)
				continue;
			e.hits++;
			JITExec out;
			out.fn		 = e.fn;
			out.chain_fn = e.chain_fn;
			out.count	 = e.count;
			return out;
		}
		return JITExec{};
	}

	bool JIT_Context::text_hash_phys(runner::Hart& h, uint64_t phys, uint32_t len, uint64_t out[2])
	{
		MemoryMap* mm = h.get_mmap();
		if(mm == nullptr || len == 0)
			return false;
		rv64vm::runner::MemoryMap::MemoryRegion* r;
		try
		{
			r = mm->find_region(phys);
		}
		catch(...)
		{
			return false;
		}
		if(r == nullptr || phys < r->get_base_addr() || (phys - r->get_base_addr()) + len > r->get_size())
			return false;
		const uint8_t* p = r->get_data() + (phys - r->get_base_addr());
		// Two independent seeded FNV-1a passes: a 128-bit digest keeps the
		// "text unchanged" decision practically exact.
		static constexpr uint64_t FNV = 0x100000001b3ULL;
		uint64_t a = 0xcbf29ce484222325ULL;
		uint64_t b = 0x9ddfea08eb382d69ULL;
		for(uint32_t i = 0; i < len; i++)
		{
			a ^= p[i];
			a *= FNV;
			b ^= p[i];
			b *= FNV;
		}
		a ^= len;
		a *= FNV;
		b ^= (uint64_t)len << 1;
		b *= FNV;
		out[0] = a;
		out[1] = b;
		return true;
	}

	bool JIT_Context::salvage(runner::Hart& h, uint64_t phys_pc, uint8_t eff_mode, bool mxr, bool sum, JITExec& out)
	{
		const size_t base	 = index_of(phys_pc) * CACHE_WAYS;
		const uint64_t epoch = g_smc_epoch.load(std::memory_order_relaxed);
		const uint64_t agen	 = g_arena_gen.load(std::memory_order_relaxed);
		for(size_t w = 0; w < CACHE_WAYS; w++)
		{
			CachedBlock& e = cache[base + w];
			if(!e.valid || e.start_phys != phys_pc)
				continue;
			if(e.eff_mode != eff_mode || e.mxr != mxr || e.sum != sum)
				continue; // baked permission policy differs; must re-emit
			if(e.smc_epoch == epoch)
				continue; // fresh; not what we're here for
			if(e.arena_gen != agen)
				continue; // its code was freed (UAF guard)
			uint64_t cur[2];
			if(!text_hash_phys(h, phys_pc, e.guest_bytes, cur))
				continue; // unreadable text: let compile re-emit
			if(e.text_hash[0] != cur[0] || e.text_hash[1] != cur[1])
				continue; // text really changed: recompiling is mandatory
			e.smc_epoch = epoch; // text verified identical: re-key, keep code
			// Re-keying makes this code live at the current epoch, so the
			// per-page code generation must follow; smc_store_hit() classifies
			// a page as invalidated by comparing g_code_page_gen[page] against
			// g_smc_epoch, and leaving the old epoch here would make it skip the
			// invalidation and run stale compiled code.
			{
				const uint64_t p0 = phys_pc & ~0xFFFULL;
				const uint64_t p1 = (phys_pc + e.guest_bytes + 0xFFF) & ~0xFFFULL;
				for(uint64_t p = p0; p < p1; p += 0x1000)
					mark_page_code(p);
			}
			e.hits++;
			out.fn		 = e.fn;
			out.chain_fn = e.chain_fn;
			out.count	 = e.count;
			return true;
		}
		return false;
	}

	bool JIT_Context::hot_tick(uint64_t phys_pc)
	{
		const size_t i = index_of(phys_pc);
		GiveUpSlot& g  = giveup[i];
		if(g.skip && g.phys == phys_pc)
			return false; // known non-JIT starter: let the interpreter have it
		HotSlot& hs		   = hotmap[i];
		const uint64_t cur_epoch = g_smc_epoch.load();
		// after an invalidation (smc epoch bump) the first dispatch to a pc
		// re-bases its counter, so one-touch (cold) code is NOT recompiled
		// after every SFENCE while genuinely hot code still recompiles.
		if(hs.hot_epoch != (uint32_t)cur_epoch)
		{
			hs.hot_epoch = (uint32_t)cur_epoch;
			hs.hot		= 0;
		}
		return ++hs.hot >= RVJIT_HOT_THRESHOLD;
	}

	bool JIT_Context::is_giveup(uint64_t phys_pc)
	{
		const size_t i = index_of(phys_pc);
		const GiveUpSlot& g = giveup[i];
		return g.skip && g.phys == phys_pc;
	}

	JITExec JIT_Context::compile(Hart& h, uint64_t va_pc, uint64_t phys_pc)
	{
		ensure_chain_dispatcher();
		while(mtx.test_and_set(std::memory_order_acquire))
		{ /* spin */
		}
		struct SpinGuard
		{
			std::atomic_flag& f;
			~SpinGuard() { f.clear(std::memory_order_release); }
		} guard{ mtx };

		// Effective access mode (MPRV honored) and the paging flags that the
		// block policy is baked from; dispatch re-validates them.
		const uint8_t eff_mode = (uint8_t)h.get_effective_mode(AccessType::STORE);
		const bool mxr		   = h.status.fields.MXR;
		const bool sum		   = h.status.fields.SUM;

		// Already compiled by a concurrent hart?
		JITExec fast = lookup(phys_pc, eff_mode, mxr, sum);
		if(fast.fn != nullptr)
			return fast;

		// Decode the guest stream into a straight-line ALU block
		JIT_Block blk;
		blk.start_phys = phys_pc;
		JIT_Emitter em(&blk);
		em.eff_mode = eff_mode;
		em.mxr		= mxr;
		em.sum		= sum;
		em.emit_prologue();

		uint64_t pc_va = va_pc;
		uint32_t count = 0;
		uint32_t size  = 0;
		// The whole block must live in the start's 4K VA page: dispatch
		// validates the VA->PA resolution only for the start pc, so page
		// containment is what lets the phys key identify the instruction
		// stream for every other asid / VA alias mapping that start.  It also
		// keeps text_hash_phys()'s contiguous phys-range digest honest.
		const uint64_t page_end = (va_pc & ~0xFFFULL) + 0x1000;
		uint32_t guest_words[RVJIT_MAX_INSTRUCTIONS];
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
			if(pc_va + cache->data.size > page_end)
			{
				break; // would straddle the page: next page's phys is dispatch-unverified
			}
			if(phys != phys_pc + size)
			{
				break; // not physically contiguous: the phys range would not cover the block
			}
			if(cache->inst->jit_func == nullptr)
				break; // not JIT-able (control/mem/system/fence/jmp/ebreak)
			// jit_func always emits before returning; count it even when it
			// reports buffer exhaustion (keep==false just ends the block).
			blk.instr_index	   = count;
			blk.instr_bytes	   = size;
			const bool keep	   = cache->inst->jit_func(h, const_cast<InstructionData&>(cache->data), blk, em);
			guest_words[count] = cache->data.inst;
			count++;
			pc_va += cache->data.size;
			size += cache->data.size;
			if(!keep)
				break; // compiled, but the block ends after it
		}

		if(count < RVJIT_MIN_INSTRUCTIONS)
		{
			// Not a JIT-able starter; make hot_tick() stop retrying it.
			GiveUpSlot& g = giveup[index_of(phys_pc)];
			g.phys		  = phys_pc;
			g.skip		  = true;
			return {};
		}

		if(!em.exited)
			em.emit_epilogue(size, count);
		em.emit_link_stubs();
		em.emit_miss_stubs();
		blk.count		= count;
		blk.bytes_guest = size;
		blk.smc_epoch	= g_smc_epoch.load();

		// Materialize into an executable (writable) arena, then hang this
		// block's per-exit private link slots right after the code. Their
		// addresses are baked into the exit tails RIP-relatively, so the
		// lea displacements must be located to the final arena address.
		const uint32_t data_base = (blk.code.pos + 15u) & ~15u;
		const uint32_t slot_span = blk.n_exits * x86::CHAIN_CACHE_STRIDE;
		uint8_t* dst = arena_alloc(data_base + slot_span);
		if(dst == nullptr)
			return {};
		memcpy(dst, blk.code.bytes, blk.code.pos);
		uint8_t* slot = dst + data_base;
		memset(slot, 0, slot_span);
		for(uint32_t i = 0; i < blk.n_exits; i++, slot += x86::CHAIN_CACHE_STRIDE)
		{
			// lea r15, [rip + disp]; disp = slot - (lea_instr + 7)
			const uint8_t* lea_inst = dst + blk.exits[i].lea_disp_off - 3;
			const int32_t rel = (int32_t)((int64_t)slot - (int64_t)(lea_inst + 7));
			memcpy(dst + blk.exits[i].lea_disp_off, &rel, 4);
		}

		// Extend the self-modifying-code protection over the block's pages.
		const uint64_t block_epoch = g_smc_epoch.load();
		mark_block_executed(phys_pc, blk.bytes_guest, h.get_mmu().get_tlb());

		const size_t base	  = index_of(phys_pc) * CACHE_WAYS;
		size_t victim		  = 0;
		uint32_t min_hits	  = UINT32_MAX;
		for(size_t w = 0; w < CACHE_WAYS; w++)
		{
			CachedBlock& e = cache[base + w];
			if(!e.valid)
			{
				victim = w;
				break; // prefer a free way
			}
			if(e.hits < min_hits)
			{
				min_hits = e.hits;
				victim	 = w;
			}
		}
		CachedBlock& e = cache[base + victim];
		e.fn		   = (JITCompiledFunc)(void*)dst;
		e.chain_fn	   = (JITCompiledFunc)(void*)(dst + blk.chain_off);
		e.start_phys   = phys_pc;
		e.smc_epoch	   = block_epoch;
		e.eff_mode	   = eff_mode;
		e.mxr		   = mxr;
		e.sum		   = sum;
		e.count		   = count;
		e.guest_bytes  = size;
		e.arena_gen	   = g_arena_gen.load(std::memory_order_relaxed);
		if(!text_hash_phys(h, phys_pc, size, e.text_hash))
		{
			// Unreadable text now: salvage() can never verify it, so every
			// invalidation will recompile - correctness unaffected.
			e.text_hash[0] = e.text_hash[1] = 0;
		}
		e.hits		   = 0;
		e.valid		   = true;

		return { e.fn, e.chain_fn, count };
	}

	void JIT_Context::mark_block_executed(uint64_t phys_pc, uint64_t guest_bytes, runner::TLB& tlb)
	{
		const uint64_t p0 = phys_pc & ~0xFFFULL;
		const uint64_t p1 = (phys_pc + guest_bytes + 0xFFF) & ~0xFFFULL;
		for(uint64_t p = p0; p < p1; p += 0x1000)
		{
			mark_page_executed(p);
			mark_page_code(p);
			// W^X: this is a PHYSICAL page address, while the TLB is
			// direct-mapped by virtual address.  note_exec() finds the entries
			// by physical coverage itself, which also covers every VA aliasing
			// the page and any huge page over it.
			tlb.note_exec(p);
		}
	}

	void JIT_Context::invalidate_all()
	{
		g_smc_epoch.fetch_add(1, std::memory_order_release);
	}

}

#endif
