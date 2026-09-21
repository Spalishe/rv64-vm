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
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
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
		for(uint8_t* a : arenas)
			munmap(a, JIT_ARENA_BYTES);
		arenas.clear();
		cur		 = nullptr;
		cur_used = 0;
	}

	// One shared chain dispatcher per process: every block exit lands here
	// (via emit.cpp's chain tail) instead of ret-ing into C++. It resolves the
	// exit pc through the direct-mapped hctx jump cache and either jumps into
	// the chained block's post-prologue entry or, on any key mismatch or cadence
	// budget exhaustion, pops the single C++ frame and returns to the runner.
	// R12 (ctx) is preserved here; the pooled/scratch regs are dead at block end.
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

		// entry = &ctx->chain_cache[(((uint32_t)(exit_pc >> 1) * 0x9E3779B9u) >> 16 & MASK) * STRIDE]
		x86::mov_mr(cb, x86::REG_RCX, x86::REG_CTX, x86::CTX_OFF_EXIT);
		x86::mov_rr(cb, x86::REG_RAX, x86::REG_RCX);
		x86::shift_r64_imm(cb, 5, x86::REG_RAX, 1);
		x86::imul_r_imm32(cb, x86::REG_RAX, (int32_t)0x9E3779B9);
		x86::shift_r64_imm(cb, 5, x86::REG_RAX, 16);
		x86::and_imm(cb, x86::REG_RAX, x86::CHAIN_CACHE_MASK);
		x86::mov_rr(cb, x86::REG_RDX, x86::REG_RAX);
		x86::shift_r64_imm(cb, 4, x86::REG_RAX, 4);
		x86::shift_r64_imm(cb, 4, x86::REG_RDX, 5);
		x86::add_rr(cb, x86::REG_RAX, x86::REG_RDX);
		x86::add_rr(cb, x86::REG_RAX, x86::REG_CTX);
		x86::add_imm(cb, x86::REG_RAX, x86::CTX_OFF_CHAIN_CACHE);

		// chain_fn != 0, pc == exit_pc
		x86::mov_mr(cb, x86::REG_R11, x86::REG_RAX, 0);
		x86::test_rr(cb, x86::REG_R11, x86::REG_R11);
		const uint32_t j_nofn = x86::jcc32(cb, 0x4); // je
		x86::cmp_r64_m64(cb, x86::REG_RCX, x86::REG_RAX, 8);
		const uint32_t j_pc = x86::jcc32(cb, 0x5); // jne

		// gen / smc / mode_key / asid against the current dispatch snapshot
		x86::mov_mr(cb, x86::REG_RDX, x86::REG_RAX, 16);
		x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_TLB_GEN);
		const uint32_t j_gen = x86::jcc32(cb, 0x5);
		x86::mov_mr(cb, x86::REG_RDX, x86::REG_RAX, 24);
		x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_SMC_KEY);
		const uint32_t j_smc = x86::jcc32(cb, 0x5);
		x86::mov_mr(cb, x86::REG_RDX, x86::REG_RAX, 32);
		x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_MODE_KEY);
		const uint32_t j_mode = x86::jcc32(cb, 0x5);
		x86::movzx_r64_m16(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_SATP_ASID);
		x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_RAX, 40);
		const uint32_t j_asid = x86::jcc32(cb, 0x5);

		// hit: re-stamp the calling block's private exit slot (r15 = its base,
		// set by the exit tail) so subsequent traversals hop directly without
		// the dispatcher; the keys mirrored here are exactly the ones the exit
		// tail re-checks, so the fast path can never outlive the validation.
		if(getenv("JDIS")) // BISECT: disable private-slot refill
		{ } else {
		x86::mov_mr(cb, x86::REG_RDX, x86::REG_RAX, 16);
		x86::mov_rm(cb, x86::REG_R15, 16, x86::REG_RDX);
		x86::mov_mr(cb, x86::REG_RDX, x86::REG_RAX, 24);
		x86::mov_rm(cb, x86::REG_R15, 24, x86::REG_RDX);
		x86::mov_mr(cb, x86::REG_RDX, x86::REG_RAX, 32);
		x86::mov_rm(cb, x86::REG_R15, 32, x86::REG_RDX);
		x86::movzx_r64_m16(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_SATP_ASID);
		x86::mov_rm(cb, x86::REG_R15, 40, x86::REG_RDX);
		x86::mov_rm(cb, x86::REG_R15, 0, x86::REG_R11);
		x86::mov_rm(cb, x86::REG_R15, 8, x86::REG_RCX);
		}

		// hit: set the target block's base pc (exits are computed as
		// entry_pc + delta_va) and jump in right after its prologue
		x86::mov_rm(cb, x86::REG_CTX, x86::CTX_OFF_ENTRY, x86::REG_RCX);
		x86::jmp_r(cb, x86::REG_R11);

		// miss: pop the single C++ frame and return to the runner, which
		// re-dispatches (the exit tail already accounted its own budget).
		const uint32_t jccs[6] = { j_nofn, j_pc, j_gen, j_smc, j_mode, j_asid };
		const uint32_t tail	 = cb.pos;
		x86::pop_r(cb, x86::REG_R12);
		x86::pop_r(cb, x86::REG_R13);
		x86::pop_r(cb, x86::REG_R15);
		x86::pop_r(cb, x86::REG_RBP);
		x86::ret(cb);

		for(int i = 0; i < 6; i++)
			x86::patch_rel32(cb, jccs[i], tail);

		memcpy(p, cb.bytes, cb.pos);
		if(x86::chain_dispatcher() == 0)
			x86::chain_dispatcher() = (uint64_t)dispatch;
		if(getenv("JDUMP"))
		{
			FILE* f = fopen("/tmp/opencode/disp.bin", "wb");
			if(f) { fwrite(cb.bytes, 1, cb.pos, f); fclose(f); }
		}
		return dispatch;
	}

	JITExec JIT_Context::lookup(uint64_t phys_pc, uint64_t asid, uint8_t eff_mode, bool mxr, bool sum)
	{
		CachedBlock& e = cache[index_of(phys_pc)];
		JITExec out;
		// asid guard keeps a VA->PA resolution honest across guest processes:
		// exec reuses a VA with a different mapping, and a recycled physical
		// page can carry stale compiled text for another asid.
		if(e.valid && e.start_phys == phys_pc && e.asid == asid && e.smc_epoch == g_smc_epoch.load() && e.eff_mode == eff_mode && e.mxr == mxr && e.sum == sum)
		{
			out.fn		 = e.fn;
			out.chain_fn = e.chain_fn;
			out.count	 = e.count;
			g_lookup_ok.fetch_add(1, std::memory_order_relaxed);
		}
		else if(e.valid && e.start_phys != phys_pc)
			g_miss_collide.fetch_add(1, std::memory_order_relaxed);
		else if(e.valid && e.start_phys == phys_pc && e.smc_epoch != g_smc_epoch.load())
			g_miss_smc.fetch_add(1, std::memory_order_relaxed);
		else if(e.valid && e.start_phys == phys_pc)
			g_miss_asid_mode.fetch_add(1, std::memory_order_relaxed);
		else
			g_miss_invalid.fetch_add(1, std::memory_order_relaxed);
		return out;
	}

	bool JIT_Context::hot_tick(uint64_t phys_pc)
	{
		static const uint32_t hot_threshold = [] {
			const char* s = getenv("RVJIT_HOT");
			return s ? (uint32_t)atoi(s) : (uint32_t)RVJIT_HOT_THRESHOLD;
		}();
		const size_t i = index_of(phys_pc);
		GiveUpSlot& g  = giveup[i];
		if(g.skip && g.phys == phys_pc)
			return false; // known non-JIT starter: let the interpreter have it
		CachedBlock& e			 = cache[i];
		const uint64_t cur_epoch = g_smc_epoch.load();
		// after an invalidation (smc epoch bump) the first
		// dispatch to a pc re-bases its counter, so one-touch (cold) code is
		// NOT recompiled after every SFENCE while genuinely hot code still
		// recompiles after 2 dispatches.
		if(e.hot_epoch != (uint32_t)cur_epoch)
		{
			e.hot_epoch = (uint32_t)cur_epoch;
			e.hot		= 0;
		}
		return ++e.hot >= hot_threshold;
	}

	bool JIT_Context::is_giveup(uint64_t phys_pc)
	{
		const size_t i = index_of(phys_pc);
		const GiveUpSlot& g = giveup[i];
		return g.skip && g.phys == phys_pc;
	}

	JITExec JIT_Context::compile(Hart& h, uint64_t va_pc, uint64_t phys_pc)
	{
		g_compile_count.fetch_add(1, std::memory_order_relaxed);
		struct CompileTimer
		{
			std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
			~CompileTimer()
			{
				g_compile_ns.fetch_add((uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
										   std::chrono::steady_clock::now() - t0)
										   .count(),
									   std::memory_order_relaxed);
			}
		} ctimer;
		ensure_chain_dispatcher();
		while(mtx.test_and_set(std::memory_order_acquire))
		{ /* spin */
		}
		struct SpinGuard
		{
			std::atomic_flag& f;
			~SpinGuard() { f.clear(std::memory_order_release); }
		} guard{ mtx };
		{
			static std::unordered_set<uint64_t> uniq;
			uniq.insert(phys_pc);
			g_unique_blocks.store(uniq.size(), std::memory_order_relaxed);
		}
		// Effective access mode (MPRV honored) and the paging flags that the
		// block policy is baked from; dispatch re-validates them.
		const uint8_t eff_mode = (uint8_t)h.get_effective_mode(AccessType::STORE);
		const bool mxr		   = h.status.fields.MXR;
		const bool sum		   = h.status.fields.SUM;

		// Already compiled by a concurrent hart?
		JITExec fast = lookup(phys_pc, h.satp.fields.asid, eff_mode, mxr, sum);
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
			if(cache->inst->jit_func == nullptr)
				break; // not JIT-able (control/mem/system/fence/jmp/ebreak)
			// jit_func always emits before returning; count it even when it
			// reports buffer exhaustion (keep==false just ends the block).
			blk.instr_index	   = count;
			blk.instr_bytes	   = size;
			blk.tmp_va		   = pc_va;
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
			static int logged_gv = 0;
			if(getenv("JLOG") && logged_gv++ < 20)
				fprintf(stderr, "giveup va=%llx phys=%llx\n", (unsigned long long)va_pc, (unsigned long long)phys_pc);
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
		blk.asid		= h.satp.fields.asid;
		blk.smc_epoch	= g_smc_epoch.load();

		// Materialize into an executable (writable) arena, then hang this
		// block's per-exit private link slots right after the code. Their
		// addresses are baked into the exit tails RIP-relatively, so the
		// lea displacements must be located to the final arena address.
		const uint32_t data_base = (blk.code.pos + 15u) & ~15u;
		uint8_t* dst = arena_alloc(data_base + blk.n_exits * 48);
		if(dst == nullptr)
			return {};
		memcpy(dst, blk.code.bytes, blk.code.pos);
		uint8_t* slot = dst + data_base;
		memset(slot, 0, (size_t)blk.n_exits * 48);
		for(uint32_t i = 0; i < blk.n_exits; i++, slot += 48)
		{
			// lea r15, [rip + disp]; disp = slot - (lea_instr + 7)
			const uint8_t* lea_inst = dst + blk.exits[i].lea_disp_off - 3;
			const int32_t rel = (int32_t)((int64_t)slot - (int64_t)(lea_inst + 7));
			memcpy(dst + blk.exits[i].lea_disp_off, &rel, 4);
		}
		if(getenv("JDUMP"))
		{
			static int ndump = 0;
			if(ndump < 32)
			{
				char fn[64];
				snprintf(fn, sizeof(fn), "/tmp/opencode/blk%d.bin", ndump);
				FILE* f = fopen(fn, "wb");
				if(f) { fwrite(blk.code.bytes, 1, blk.code.pos, f); fclose(f); }
				snprintf(fn, sizeof(fn), "/tmp/opencode/blk%d.txt", ndump);
				FILE* g = fopen(fn, "w");
				if(g)
				{
					fprintf(g, "dst=%p data_base=%u n_exits=%u\n", (void*)dst, data_base, blk.n_exits);
					for(uint32_t i = 0; i < blk.n_exits; i++)
						fprintf(g, "exit %u data_idx=%u lea_disp_off=%u budget_js=%u fail=%d %d %d %d %d slot=%p\n",
								i, blk.exits[i].data_idx, blk.exits[i].lea_disp_off, blk.exits[i].budget_js,
								blk.exits[i].fail[0], blk.exits[i].fail[1], blk.exits[i].fail[2],
								blk.exits[i].fail[3], blk.exits[i].fail[4],
								(void*)(dst + data_base + i * 48));
					fclose(g);
				}
				ndump++;
			}
		}

		// Extend the self-modifying-code protection over the block's pages.
		const uint64_t block_epoch = g_smc_epoch.load();
		mark_block_executed(phys_pc, blk.bytes_guest);

		// W^X: the inline TLB check honors each entry's write permission, so
		// executed pages must not advertise W|D - otherwise a JITed store would
		// slip past the self-modifying-code detector. Strip the covering slots.
		{
			const uint64_t p0 = phys_pc & ~0xFFFULL;
			const uint64_t p1 = (phys_pc + blk.bytes_guest + 0xFFF) & ~0xFFFULL;
			for(uint64_t p = p0; p < p1; p += 0x1000)
				h.get_mmu().get_tlb().note_exec(p);
		}

		CachedBlock& e = cache[index_of(phys_pc)];
		e.fn		   = (JITCompiledFunc)(void*)dst;
		e.chain_fn	   = (JITCompiledFunc)(void*)(dst + blk.chain_off);
		e.start_phys   = phys_pc;
		e.asid		   = h.satp.fields.asid;
		e.smc_epoch	   = block_epoch;
		e.eff_mode	   = eff_mode;
		e.mxr		   = mxr;
		e.sum		   = sum;
		e.count		   = count;
		e.valid		   = true;

		return { e.fn, e.chain_fn, count };
	}

	void JIT_Context::mark_block_executed(uint64_t phys_pc, uint64_t guest_bytes)
	{
		const uint64_t p0 = phys_pc & ~0xFFFULL;
		const uint64_t p1 = (phys_pc + guest_bytes + 0xFFF) & ~0xFFFULL;
		for(uint64_t p = p0; p < p1; p += 0x1000)
		{
			mark_page_executed(p);
			mark_page_code(p);
		}
	}

	void JIT_Context::invalidate_all()
	{
		g_inval_count.fetch_add(1, std::memory_order_relaxed);
		g_smc_epoch.fetch_add(1, std::memory_order_release);
	}
}

#endif
