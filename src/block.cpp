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

#include "../include/block_cache.hpp"
#include "../include/hart.hpp"
#include <chrono>
#include <cstdlib>
#ifdef USE_JIT
#include "../include/jit/rvjit.hpp"
#include "../include/self_mod.hpp"
#endif

namespace rv64vm::runner
{
#ifdef USE_JIT
	const bool RTRACE = getenv("RTRACE") != nullptr; // one-time, keeps the hot
	const bool JPROF	= getenv("JPROF") != nullptr; // dispatch loop env-free
#endif
	namespace
	{
		std::atomic<uint64_t> g_dispatch{ 0 };	   // JIT chain calls from run_blocks
		std::atomic<uint64_t> g_dispatch_zero{ 0 }; // ...that ran zero instructions
		std::atomic<uint64_t> g_interp_insts{ 0 }; // instructions run by the block interpreter
		std::atomic<uint64_t> g_memo_hit{ 0 };	 // dhot memo hits
		std::atomic<uint64_t> g_memo_fn{ 0 };	 // memo hits carrying a compiled fn
		std::atomic<uint64_t> g_dhot_tick{ 0 };	 // global pseudo-LRU generation for memo ways
		std::atomic<uint64_t> g_interp_disp{ 0 }; // dispatches that fell through to the interpreter
		std::atomic<uint64_t> g_interp_memo{ 0 }; // ...served by the memoized giveup path

		// Ops that unconditionally write GPR[rd]; a rd==0 one ends the block so
		// x0 is never corrupted mid-block.
		inline bool writes_reg(uint32_t inst)
		{
			switch(inst & 0x7F)
			{
				case 0x13: // OP-IMM
				case 0x33: // OP
				case 0x37: // LUI
				case 0x17: // AUIPC
				case 0x6F: // JAL
				case 0x67: // JALR
				case 0x03: // loads
					return true;
				default:
					return false;
			}
		}

		// Blocks must end on any control transfer / system op so the runner
		// re-dispatches with consistent pc/mode/gpr state.
		inline bool is_block_end(uint32_t inst)
		{
			switch(inst & 0x7F)
			{
				case 0x63: // branches
				case 0x6F: // JAL
				case 0x67: // JALR
				case 0x73: // SYSTEM
				case 0x0F: // FENCE / FENCE.I
					return true;
				default:
					break;
			}
			if((inst & 0x3) != 0x3) // compressed control ops
			{
				if((inst & 0x3) == 0x1)
				{
					// C.J / C.JAL / C.BEQZ / C.BNEZ
					const uint32_t f3 = (inst >> 13) & 0x7;
					if(f3 == 0x5 || f3 == 0x1 || f3 == 0x6 || f3 == 0x7)
						return true;
				}
				else
				{
					// C.JR / C.JALR / C.EBREAK: funct3=100, rs2 field == 0
					// (this excludes C.MV which shares the top bits but has a rs2)
					if((inst & 0x7F) == 0x02 && (inst & 0xE003) == 0x8002)
						return true;
				}
			}
			return false;
		}

		// On fault, `out_executed` credits the finished instructions and h.pc
		// points at the faulting instruction, like the interpreter.
		inline void run_block(Hart& h, Block& b, uint64_t& out_executed, bool& out_fault, uint32_t& cause, uint64_t& tval)
		{
			h.GPR[0]		   = 0; // a block may leave x0 non-zero if its last op wrote rd=0
			const uint32_t cnt = b.count;
			uint32_t n		   = 0;
			for(; n + 1 < cnt; n++)
			{
				const BlockInstr& ci = b.instrs[n];
				ExecReturn r		 = ci.inst->func(h, const_cast<InstructionData&>(ci.data));
				if(!r.is_success) [[unlikely]]
				{
					out_fault	 = true;
					cause		 = r.cause;
					tval		 = r.tval;
					out_executed = n;
					return;
				}
				h.pc += r.increase_pc;
				if(h.GPR[0] != 0) [[unlikely]]
					h.GPR[0] = 0;
			}
			const BlockInstr& ci = b.instrs[cnt - 1];
			ExecReturn r		 = ci.inst->func(h, const_cast<InstructionData&>(ci.data));
			if(!r.is_success) [[unlikely]]
			{
				out_fault	 = true;
				cause		 = r.cause;
				tval		 = r.tval;
				out_executed = cnt - 1;
				return;
			}
			h.pc += r.increase_pc;
			out_executed = cnt;
		}
	}

	Block* Hart::compile_block(BlockCache& bc, uint64_t start_phys)
	{
		Block& b	 = bc.slots[(start_phys >> 2) & (BlockCache::CACHE_SIZE - 1)];
		b.gen		 = 0; // invalidate until fully built
		b.start_phys = start_phys;

		uint64_t pc_va = pc;
		uint32_t n	   = 0;
		while(n < BLOCK_MAX_INSTS)
		{
			uint64_t phys;
			InstructionCache* cache;
			MemoryReturn mr = fetchInstruction(pc_va, phys, cache);
			if(!mr.is_success)
				break;
			if(cache->pc == 0) // illegal instruction
				break;
			const uint32_t inst = cache->data.inst;
			if((inst & 0x3) == 0x3 && writes_reg(inst) && ((inst >> 7) & 0x1F) == 0)
				break; // rd == 0 would poison x0 for the rest of the block

			BlockInstr& bi = b.instrs[n];
			bi.inst		   = cache->inst;
			bi.data		   = cache->data;
			bi.increase_pc = ((inst & 0x3) == 0x3) ? 4 : 2;
			b.last_inst	   = inst;
			n++;
			if(is_block_end(inst))
				break;
			pc_va += bi.increase_pc;
		}
		if(n == 0)
			return nullptr;
		b.count = n;
		b.gen	= bc.generation;
		b.smc	= g_smc_epoch.load();
		mark_page_executed(start_phys);
		mark_page_executed(start_phys + (uint64_t)n * 4);
		return &b;
	}

	uint64_t Hart::run_blocks(BlockCache& bc, uint64_t max_insts)
	{
		if(WFI) [[unlikely]]
		{
			if(int_local_pending()) WFI = false;
			return 0;
		}

		uint64_t total = 0;
		for(;;)
		{
			if(WFI) [[unlikely]]
			{
				if(int_local_pending()) WFI = false;
				break;
			}
			if(total >= max_insts)
				break;

			if(JPROF)
			{
				static bool init_mark = false;
				if(!init_mark && pc >= 0xffffffff80002078ULL && pc < 0xffffffff80002116ULL)
				{
					init_mark = true;
					auto now = std::chrono::steady_clock::now();
					uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
					fprintf(stderr, "MARK init instret=%llu ns=%llu compiles=%llu uniq=%llu ctime_ms=%llu invals=%llu smc=%llu ok=%llu coll=%llu msmc=%llu masid=%llu minv=%llu disp=%llu z=%llu gf=%llu mm=%llu interp=%llu walk=%llu fc=%llu mh=%llu mfn=%llu idisp=%llu imemo=%llu\n", (unsigned long long)instret, (unsigned long long)ns, (unsigned long long)jit::g_compile_count.load(), (unsigned long long)jit::g_unique_blocks.load(), (unsigned long long)(jit::g_compile_ns.load()/1000000), (unsigned long long)jit::g_inval_count.load(), (unsigned long long)rv64vm::g_smc_epoch.load(), (unsigned long long)jit::g_lookup_ok.load(), (unsigned long long)jit::g_miss_collide.load(), (unsigned long long)jit::g_miss_smc.load(), (unsigned long long)jit::g_miss_asid_mode.load(), (unsigned long long)jit::g_miss_invalid.load(), (unsigned long long)g_dispatch.load(), (unsigned long long)g_dispatch_zero.load(), (unsigned long long)jit::x86::g_chain_guardfail.load(), (unsigned long long)jit::x86::g_chain_memmiss.load(), (unsigned long long)g_interp_insts.load(), (unsigned long long)g_walk_count.load(), (unsigned long long)rv64vm::runner::g_flush_count.load(), (unsigned long long)g_memo_hit.load(), (unsigned long long)g_memo_fn.load(), (unsigned long long)g_interp_disp.load(), (unsigned long long)g_interp_memo.load());
				}
			}

			if((instret & 0x2FFF) == 0) [[unlikely]] // interrupt cadence, mirrors tick()
			{
				if((ip.raw & ie.raw) != 0 && check_ints())
					break;
			}

		uint64_t phys	= 0;
		bool phys_done = false;
#ifdef USE_JIT
		if(jctx != nullptr && (pc & 0x1) == 0) [[likely]]
		{
			const uint64_t gen  = mmu.get_tlb().current_generation();
			const uint8_t  mode = (uint8_t)get_effective_mode(AccessType::EXEC);
			uint64_t smc  = rv64vm::g_smc_epoch.load(std::memory_order_relaxed);
			const uint32_t asid = satp.fields.asid;
			const size_t di = ((pc >> 1) * 2654435761u + (uint32_t)gen + (uint32_t)smc) & 4095;
			jit::JITExec jj{};
			// Chain key: mirrors what compile() bakes into block TLB checks.
			const uint64_t mode_key = (uint64_t)get_effective_mode(AccessType::STORE) |
									  (status.fields.MXR ? 0x100ull : 0) |
									  (status.fields.SUM ? 0x200ull : 0);

			// Set-associative memo: scan the bucket's ways for a keyed hit, or
			// install into a free way / the least-recent one.  A hit with a
			// compiled fn skips the translate+lookup on the re-dispatch.
			jit::JITExec cached{};
			DispatchHot* dhotw = &dhot[di * MEMO_WAYS];
			DispatchHot* d	   = nullptr;
			for(int widx = 0; widx < MEMO_WAYS; widx++)
			{
				DispatchHot& s = dhotw[widx];
				if(s.seen && s.va == pc && s.gen == gen && s.mode == mode && s.smc == smc && s.asid == asid)
				{
					d = &s;
					s.lru = (uint8_t)g_dhot_tick.fetch_add(1, std::memory_order_relaxed);
					break;
				}
			}
			if(d == nullptr)
			{
				int slot = 0;
				uint8_t min_lru = 0xFF;
				for(int widx = 0; widx < MEMO_WAYS; widx++)
				{
					DispatchHot& s = dhotw[widx];
					if(!s.seen)
					{
						slot = widx;
						break;
					}
					if(s.lru < min_lru)
					{
						min_lru = s.lru;
						slot	= widx;
					}
				}
				d = &dhotw[slot];
				d->va		= pc;
				d->gen		= gen;
				d->mode		= mode;
				d->smc		= smc;
				d->asid		= asid;
				d->seen		= true;
				d->fn		= nullptr;
				d->chain_fn = nullptr;
				d->interp	= 0;
				d->lru		= (uint8_t)g_dhot_tick.fetch_add(1, std::memory_order_relaxed);
			}
			const bool memo_hit = true; // `d` always the exact key now
			bool interp_memo	  = false;
			if(JPROF) g_memo_hit.fetch_add(1, std::memory_order_relaxed);
			if(memo_hit && d->fn != nullptr)
			{
				cached.fn		= d->fn;
				cached.chain_fn = d->chain_fn;
				phys			= d->phys;
				phys_done		= true;
				if(JPROF) g_memo_fn.fetch_add(1, std::memory_order_relaxed);
			}
			else if(memo_hit && d->interp)
			{
				// Permanent interpreter fallback (giveup block): the translation
				// and JIT-decision are still valid for this key; reuse phys and
				// skip translate + lookup + hot_tick entirely.
				phys		 = d->phys;
				phys_done	 = true;
				interp_memo = true;
				if(JPROF) g_interp_memo.fetch_add(1, std::memory_order_relaxed);
			}
			// else: no compiled fn and no giveup decision yet - fall through to
			// translate + lookup + hot_tick below (the way is already keyed).

			if(!phys_done)
			{
				MemoryReturn mr = mmu.translate(this, AccessType::EXEC, pc, &phys);
				if(!mr.is_success) [[unlikely]]
				{
					trap(mr.exc_code, mr.tval, false);
					break;
				}
				phys_done = true;
			}

			jj = cached;
			if(jj.fn == nullptr && !interp_memo)
			{
				d->phys = phys;
				const uint8_t eff_mode = (uint8_t)get_effective_mode(AccessType::STORE);
				const bool mxr		   = status.fields.MXR;
				const bool sum		   = status.fields.SUM;
				jj = jctx->lookup(phys, eff_mode, mxr, sum);
				if(jj.fn == nullptr && jctx->hot_tick(phys))
				{
					// An invalidation leaves mostly-intact text; reuse the
					// compiled block when the guest bytes are unchanged.
					if(!jctx->salvage(*this, phys, eff_mode, mxr, sum, jj))
						jj = jctx->compile(*this, pc, phys);
				}
				// Re-read smc after compile() — it may have called
				// release_arenas() which bumps g_smc_epoch.  Using a
				// stale smc would let the chain dispatcher validate
				// against freed arenas.
				smc = rv64vm::g_smc_epoch.load(std::memory_order_acquire);
				d->fn		= jj.fn;
				d->chain_fn = jj.chain_fn;
				// Remember a permanent interpreter decision so repeat
				// dispatches skip the JIT machinery above.
				if(jj.fn == nullptr && jctx->is_giveup(phys))
					d->interp = 1;
			}

			if(jj.fn == nullptr)
				if(JPROF) g_interp_disp.fetch_add(1, std::memory_order_relaxed);
			if(jj.fn != nullptr)
			{
				hctx.tlb_entries = mmu.get_tlb().jit_entries();
				hctx.tlb_gen	 = gen;
				hctx.satp_asid	 = asid;
				hctx.smc_key	 = smc;
				hctx.mode_key	 = mode_key;
				hctx.chain_budget = (int64_t)jit::x86::CHAIN_CADENCE;

// Install this block into the chain jump cache so any exit
			// targeting this guest pc can hop here directly. Mirrors the
			// JIT chain dispatcher's Fibonacci-hashed index.
			const uint64_t cidx = ((uint64_t)(uint32_t)((uint64_t)pc >> 1) * 0x9E3779B9u) >> 16;
			uint64_t* ce		  = &hctx.chain_cache[(cidx & jit::x86::CHAIN_CACHE_MASK) * 6];
			ce[0] = (uint64_t)jj.chain_fn;
			ce[1] = pc;
			ce[2] = gen;
			ce[3] = smc;
			ce[4] = mode_key;
			ce[5] = (uint64_t)(uint16_t)asid;

				const uint64_t prev_instret = instret;
				hctx.entry_pc				= pc;
				if(JPROF) g_dispatch.fetch_add(1, std::memory_order_relaxed);
				jj.fn(&hctx);
				// The chain dispatcher counts every hop into chain_budget;
				// the budget released is the whole chain's instruction total.
const uint64_t executed = (uint64_t)((int64_t)jit::x86::CHAIN_CADENCE - hctx.chain_budget);
			if(executed == 0)
				if(JPROF) g_dispatch_zero.fetch_add(1, std::memory_order_relaxed);
			pc						= hctx.exit_pc;
			instret += executed;
			cycle += executed;
			total += executed;
			if(JPROF)
			{
				static uint64_t sampled = 0;
				if((++sampled & 0xFFF) == 0)
				{
					auto now = std::chrono::steady_clock::now();
					uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
					FILE* f = fopen("/tmp/opencode/prof.log", "a");
					if(f)
					{
						fprintf(f, "%llu %llu %llx\n", (unsigned long long)ns, (unsigned long long)instret, (unsigned long long)pc);
						fclose(f);
					}
					FILE* c = fopen("/tmp/opencode/counters.log", "a");
					if(c)
					{
						static uint64_t c_last_ns = 0;
						if(ns - c_last_ns >= 50000000ULL) // ~20 Hz rate cap
						{
							c_last_ns = ns;
							fprintf(c, "%llu comp=%llu uniq=%llu ctime_ms=%llu invals=%llu smc=%llu slv_ok=%llu slv_mod=%llu slv_dead=%llu slv_text=%llu slv_unr=%llu slv_none=%llu ok=%llu coll=%llu msmc=%llu masid=%llu minv=%llu disp=%llu z=%llu interp=%llu walk=%llu fc=%llu mh=%llu mfn=%llu idisp=%llu imemo=%llu\n",
								(unsigned long long)ns, (unsigned long long)jit::g_compile_count.load(), (unsigned long long)jit::g_unique_blocks.load(), (unsigned long long)(jit::g_compile_ns.load()/1000000), (unsigned long long)jit::g_inval_count.load(), (unsigned long long)rv64vm::g_smc_epoch.load(), (unsigned long long)jit::g_slv_ok.load(), (unsigned long long)jit::g_slv_mod.load(), (unsigned long long)jit::g_slv_dead.load(), (unsigned long long)jit::g_slv_text.load(), (unsigned long long)jit::g_slv_unread.load(), (unsigned long long)jit::g_slv_none.load(), (unsigned long long)jit::g_lookup_ok.load(), (unsigned long long)jit::g_miss_collide.load(), (unsigned long long)jit::g_miss_smc.load(), (unsigned long long)jit::g_miss_asid_mode.load(), (unsigned long long)jit::g_miss_invalid.load(), (unsigned long long)g_dispatch.load(), (unsigned long long)g_dispatch_zero.load(), (unsigned long long)g_interp_insts.load(), (unsigned long long)g_walk_count.load(), (unsigned long long)rv64vm::runner::g_flush_count.load(), (unsigned long long)g_memo_hit.load(), (unsigned long long)g_memo_fn.load(), (unsigned long long)g_interp_disp.load(), (unsigned long long)g_interp_memo.load());
						}
						fclose(c);
					}
				}
			}
			if(RTRACE)
			{
				static int nt = 0;
				if(pc >= 0xffffffff80000000ULL && nt++ < 300)
					fprintf(stderr, "rt: pc=%llx ex=%llu bud=%ld exit=%llx fn=%p cf=%p ge=%llx hops=%llu hopfn=%llx hpc=%llx\n",
							(unsigned long long)pc, (unsigned long long)executed,
							(long)hctx.chain_budget, (unsigned long long)hctx.exit_pc,
							(void*)jj.fn, (void*)jj.chain_fn,
							(unsigned long long)rv64vm::g_smc_epoch.load(),
							(unsigned long long)jit::x86::g_jit_hops,
							(unsigned long long)jit::x86::g_hop_fn,
							(unsigned long long)jit::x86::g_hop_pc);
			}
				if(executed != 0)
				{
					if((prev_instret & 0x2FFF) + executed >= 0x3000) [[unlikely]]
					{
						if((ip.raw & ie.raw) != 0 && check_ints())
							break;
					}
					if(total >= max_insts) [[unlikely]]
						break;
					// A JIT block exit lands on another JIT block start;
					// re-enter the dispatch instead of the interpreter.
					continue;
				}
			}
		}
#endif

		if(!phys_done)
		{
			MemoryReturn mr = mmu.translate(this, AccessType::EXEC, pc, &phys);
			if(!mr.is_success)
			{
				trap(mr.exc_code, mr.tval, false);
				break;
			}
		}

			Block* b = bc.lookup(phys);
			if(b == nullptr)
				b = compile_block(bc, phys);
			if(b == nullptr) // can't form a block: single interpreter step
			{
				tick();
				if(WFI)
					break;
				total += 1;
				continue;
			}

			uint64_t executed = 0;
			bool fault		  = false;
			uint32_t cause	  = 0;
			uint64_t tval	  = 0;
			run_block(*this, *b, executed, fault, cause, tval);
			if(JPROF) g_interp_insts.fetch_add((uint64_t)executed, std::memory_order_relaxed);

			const uint64_t prev_instret = instret;
			instret += executed;
			cycle += executed;
			total += executed;
			if(fault) [[unlikely]]
			{
				trap(cause, tval, false);
				break;
			}
			if((prev_instret & 0x2FFF) + executed >= 0x3000) [[unlikely]]
			{
				if((ip.raw & ie.raw) != 0 && check_ints())
					break;
			}

			// FENCE / FENCE.I: guest may have rewritten its own code.
			if((b->last_inst & 0x7F) == 0x0F) [[unlikely]]
				bc.clear();
		}
		return total;
	}
}
