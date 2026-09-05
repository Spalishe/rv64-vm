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

#include "../include/hart.hpp"
#include "../include/block_cache.hpp"
#include <cstdlib>
#include <cstdio>
#ifdef USE_JIT
#include "../include/jit/rvjit.hpp"
#endif

namespace rv64vm::runner
{
	namespace
	{
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
			h.GPR[0] = 0; // a block may leave x0 non-zero if its last op wrote rd=0
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
		Block& b		 = bc.slots[(start_phys >> 2) & (BlockCache::CACHE_SIZE - 1)];
		b.gen			 = 0; // invalidate until fully built
		b.start_phys	 = start_phys;

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

			BlockInstr& bi	 = b.instrs[n];
			bi.inst			 = cache->inst;
			bi.data			 = cache->data;
			bi.increase_pc	 = ((inst & 0x3) == 0x3) ? 4 : 2;
			b.last_inst		 = inst;
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

			if((instret & 0x2FFF) == 0) [[unlikely]] // interrupt cadence, mirrors tick()
			{
				if((ip.raw & ie.raw) != 0 && check_ints())
					break;
			}

			uint64_t phys = 0;
			MemoryReturn mr = mmu.translate(this, AccessType::EXEC, pc, &phys);
			if(!mr.is_success)
			{
				trap(mr.exc_code, mr.tval, false);
				break;
			}

#ifdef USE_JIT
			// Native JIT fast path. Blocks are keyed by the PHYSICAL pc and the
			// runner re-translates `pc` (VA) before every dispatch, so aliased
			// VAs and ASID switches are correct by construction. Lookups also
			// validate the ASID and self-modifying-code epoch per entry.
			if(jctx != nullptr && (pc & 0x3) == 0) [[likely]]
			{
				jit::JITExec jj = jctx->lookup(phys, satp.fields.asid);
				if(jj.fn == nullptr && jctx->hot_tick(phys))
					jj = jctx->compile(*this, pc, phys);
				if(jj.fn != nullptr)
				{
					const uint64_t prev_instret = instret;
					hctx.entry_pc = pc;
					jj.fn(&hctx);
					pc			   = hctx.exit_pc;
					instret			+= jj.count;
					cycle			+= jj.count;
					total			+= jj.count;
					if((prev_instret & 0x2FFF) + jj.count >= 0x3000) [[unlikely]]
					{
						if((ip.raw & ie.raw) != 0 && check_ints())
							break;
					}
					continue;
				}
			}
#endif

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

			uint64_t executed	  = 0;
			bool fault			  = false;
			uint32_t cause		  = 0;
			uint64_t tval		  = 0;
			run_block(*this, *b, executed, fault, cause, tval);

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