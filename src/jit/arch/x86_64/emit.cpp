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

#include "../../../../include/jit/rvjit_emit.hpp"
#include <cstdlib>

namespace rv64vm::jit
{
	namespace
	{
		// TLB::TLBPermissions bits (see tlb.hpp).
		constexpr uint8_t PERM_R = 1u << 0;
		constexpr uint8_t PERM_W = 1u << 1;
		constexpr uint8_t PERM_X = 1u << 2;
		constexpr uint8_t PERM_U = 1u << 3;
		constexpr uint8_t PERM_A = 1u << 4;
		constexpr uint8_t PERM_D = 1u << 5;

		// Upper bound for a single miss-stub (dirty-register flush stores +
		// mov/add/mov/mov64imm/pop/pop/pop/ret). Reserve conservatively so
		// eof() can stop the block before the stubs overflow the buffer.
		constexpr uint32_t STUB_BYTES = 104;

		// Upper bound for emit_epilogue(), which is appended after the decode
		// loop and is not covered by the per-instruction reserve.
		constexpr uint32_t EPILOGUE_BYTES = 256;

		constexpr uint8_t CC_JE	 = 0x4;
		constexpr uint8_t CC_JNE = 0x5;

		void emit_chain_tail(JIT_Emitter& em, uint32_t count)
		{
			// Self-contained block exit. RAX holds the exit pc (entry + delta).
			// Writes the runner's EXIT/ENTRY/EXIT_COUNT, accounts this block's
			// own slice of the chain budget, then either hops straight to the
			// successor whose private link slot (one per exit, in this block's
			// arena chunk, reached RIP-relatively through r15) matches the
			// dispatch snapshot, or falls back to the shared chain dispatcher.
			// The dispatcher re-stamps that slot on every hit, so no code
			// patching or invalidation sweep is needed: an smc/gen/asid/mode
			// change trips a guard and returns control to the C++ runner, which
			// re-dispatches with fresh keys and eventually re-stamps the slot.
			x86::CodeBuf& cb = em.code();
			x86::mov_rm(cb, x86::REG_CTX, x86::CTX_OFF_EXIT, x86::REG_RAX);
			x86::mov_rm(cb, x86::REG_CTX, x86::CTX_OFF_ENTRY, x86::REG_RAX);
			x86::mov_m64_imm(cb, x86::REG_CTX, x86::CTX_OFF_EXIT_COUNT, (int32_t)count);
			x86::sub_m64_imm(cb, x86::REG_CTX, x86::CTX_OFF_CHAIN_BUDGET, (int32_t)count);

			JIT_Block::ExitLink& L	= em.blk->exits[em.blk->n_exits];
			L.data_idx				= em.blk->n_exits;
			em.blk->n_exits++;
			L.budget_js = x86::jcc32(cb, 0x8); // js: budget exhausted -> runner

			const uint32_t lea_off = cb.pos; // lea r15, [rip + private slot]
			x86::lea_r64_rip(cb, x86::REG_R15);
			L.lea_disp_off = lea_off + 3;

			x86::mov_mr(cb, x86::REG_R11, x86::REG_R15, 0);
			x86::test_rr(cb, x86::REG_R11, x86::REG_R11);
			L.fail[0] = x86::jcc32(cb, 0x4); // je: slot empty -> dispatcher
			// The exit pc (rax = entry + delta) must match the pc the slot was
			// resolved for. A block is keyed by its PHYSICAL start, so the same
			// compiled code can be hit from two VAs mapping one page (identity
			// + linear map during the MMU switch); the site then computes a
			// different rax and hopping to the slot's chain-fn would continue
			// at the branch target of the OTHER va.
			x86::cmp_r64_m64(cb, x86::REG_RAX, x86::REG_R15, 8);
			L.fail[1] = x86::jcc32(cb, 0x5);
			x86::mov_mr(cb, x86::REG_RDX, x86::REG_R15, 16);
			x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_TLB_GEN);
			L.fail[2] = x86::jcc32(cb, 0x5);
			x86::mov_mr(cb, x86::REG_RDX, x86::REG_R15, 24);
			x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_SMC_KEY);
			L.fail[3] = x86::jcc32(cb, 0x5);
			x86::mov_mr(cb, x86::REG_RDX, x86::REG_R15, 32);
			x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_MODE_KEY);
			L.fail[4] = x86::jcc32(cb, 0x5);
			x86::movzx_r64_m16(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_SATP_ASID);
			x86::cmp_r16_m16(cb, x86::REG_RDX, x86::REG_R15, 40);
			L.fail[5] = x86::jcc32(cb, 0x5);
			if(getenv("JTRACE"))
			{
				x86::mov_imm64(cb, x86::REG_R10, (uint64_t)&x86::g_jit_hops);
				x86::mov_rm(cb, x86::REG_R10, 8, x86::REG_R11); // fn
				x86::mov_rm(cb, x86::REG_R10, 16, x86::REG_RAX); // pc
				x86::mov_mr(cb, x86::REG_RAX, x86::REG_R10, 0);
				x86::add_imm(cb, x86::REG_RAX, 1);
				x86::mov_rm(cb, x86::REG_R10, 0, x86::REG_RAX);
			}
			x86::jmp_r(cb, x86::REG_R11);
		}

		/*
		 * Inline TLB lookup, adapted from RVVM's rvjit_tlb_lookup but validated
		 * at runtime instead of recompiling on miss: every check that can fail
		 * jumps to the stub of `instr` at the end of the block, which exits to
		 * the interpreter at a precise pc. On success H ends up as the host
		 * address and the caller emits the actual memory access.
		 *
		 * Temp usage: RCX = TLB entry base, RAX/RDX scratch. H must be an
		 * allocator-pool register (never RAX/RDX/RCX).
		 */
		void emit_tlb_checks(JIT_Emitter& em, uint8_t H, uint8_t width, bool store, uint32_t instr)
		{
			x86::CodeBuf& cb	= em.code();
			auto miss_jump = [&](uint8_t cc)
			{
				em.push_miss(x86::jcc32(cb, cc), instr);
			};

			// E = [CTX + TLB_ENTRIES] + (((H >> 12) & TLB_SIZE_MASK) << 6)
			x86::mov_rr(cb, x86::REG_RCX, H);
			x86::shift_r64_imm(cb, 5, x86::REG_RCX, 12);
			x86::and_imm(cb, x86::REG_RCX, x86::TLB_SIZE_MASK);
			x86::shift_r64_imm(cb, 4, x86::REG_RCX, x86::TLB_ENTRY_LOG2);
			x86::add_r64_m64(cb, x86::REG_RCX, x86::REG_CTX, x86::CTX_OFF_TLB_ENTRIES);

			// generation: entry.generation == ctx.tlb_gen (flush detection, also
			// guards against entries installed by other harts after a flush).
			x86::mov_mr(cb, x86::REG_RDX, x86::REG_RCX, x86::TLB_OFF_GENERATION);
			x86::cmp_r64_m64(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_TLB_GEN);
			miss_jump(CC_JNE);

			// containment: (H ^ vpage_base) & vpage_mask_inv == 0
			x86::mov_rr(cb, x86::REG_RAX, H);
			x86::xor_r64_m64(cb, x86::REG_RAX, x86::REG_RCX, x86::TLB_OFF_VPAGE_BASE);
			x86::and_r64_m64(cb, x86::REG_RAX, x86::REG_RCX, x86::TLB_OFF_VPAGE_MASK_INV);
			miss_jump(CC_JNE);

			// asid: match, or the entry is global (short forward skips).
			x86::movzx_r64_m16(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_SATP_ASID);
			x86::cmp_m16_r16(cb, x86::REG_RCX, x86::TLB_OFF_ASID, x86::REG_RDX);
			const uint32_t ok_a = x86::jcc8(cb, 0x74); // je
			x86::test_m8_imm(cb, x86::REG_RCX, x86::TLB_OFF_GLOBAL, 0xFF);
			const uint32_t ok_b		 = x86::jcc8(cb, 0x75); // jne
			const uint32_t asid_miss = x86::jmp32(cb);
			em.push_miss(asid_miss, instr);
			x86::patch_rel8(cb, ok_a, cb.pos);
			x86::patch_rel8(cb, ok_b, cb.pos);

			// perm, with the mode/sum/mxr policy baked at compile time.
			uint32_t must = store ? (PERM_W | PERM_D) : (em.mxr ? (PERM_R | PERM_X) : PERM_R);
			uint32_t forb = 0;
			if(em.eff_mode == 0)
				must |= PERM_U; // user mode: page must be user-owned
			else if(em.eff_mode == 1 && !em.sum)
				forb = PERM_U; // supervisor w/o SUM: no user pages
			x86::movzx_r64_m8(cb, x86::REG_RAX, x86::REG_RCX, x86::TLB_OFF_PERM);
			x86::and_imm32(cb, x86::REG_RAX, (int32_t)(must | forb));
			x86::cmp_imm32(cb, x86::REG_RAX, (int32_t)must);
			miss_jump(CC_JNE);

			// alignment: the fast path only handles in-page, aligned access.
			if(width > 1)
			{
				x86::test_imm(cb, H, (int32_t)(width - 1));
				miss_jump(CC_JNE);
			}

			// host mapping: host_ptr != 0, then H += host_ptr.
			x86::mov_mr(cb, x86::REG_RDX, x86::REG_RCX, x86::TLB_OFF_HOST_PTR);
			x86::test_rr(cb, x86::REG_RDX, x86::REG_RDX);
			miss_jump(CC_JE);
			x86::add_rr(cb, H, x86::REG_RDX);
		}
	}
	JIT_Emitter::JIT_Emitter(JIT_Block* b) : blk(b)
	{
		for(size_t i = 0; i < x86::RVJIT_HOST_REGS; i++)
			hr_vreg[i] = 0xFF;
	}

	x86::CodeBuf& JIT_Emitter::code() const
	{
		return blk->code;
	}

	uint8_t JIT_Emitter::free_slot(uint32_t keep1, uint32_t keep2)
	{
		for(size_t i = 0; i < x86::RVJIT_HOST_REGS; i++)
		{
			if(hr_vreg[i] == 0xFF)
			{
				hr_vreg[i] = 0xFE; // owned, no guest value yet
				return (uint8_t)i;
			}
		}
		// Find a clean spill candidate (skip protected live operands).
		for(size_t i = 0; i < x86::RVJIT_HOST_REGS; i++)
		{
			uint32_t h = hr_vreg[i];
			if(h != 0xFF && h != 0xFE && h < 32 && h != keep1 && h != keep2 && !vr[h].dirty)
			{
				vr[h].slot = -1;
				hr_vreg[i] = 0xFE;
				return (uint8_t)i;
			}
		}
		// No clean slot: spill a dirty one, again skipping protected ops.
		for(size_t i = 0; i < x86::RVJIT_HOST_REGS; i++)
		{
			uint32_t h = hr_vreg[i];
			if(h != 0xFF && h != 0xFE && h < 32 && h != keep1 && h != keep2)
			{
				flush_guest(h);
				vr[h].slot = -1;
				hr_vreg[i] = 0xFE;
				return (uint8_t)i;
			}
		}
		// Unreachable: at most keep1/keep2 occupy two of the eight slots.
		__builtin_unreachable();
	}

	uint8_t JIT_Emitter::phys(uint8_t slot) const
	{
		return x86::RVJIT_HOST_POOL[slot];
	}

	uint8_t JIT_Emitter::hreg_for_read(uint32_t v, uint32_t keep)
	{
		VReg& g = vr[v];
		if(g.slot >= 0)
			return phys((uint8_t)g.slot);
		uint8_t s  = free_slot(keep);
		uint8_t p  = phys(s);
		hr_vreg[s] = (uint8_t)v;
		g.slot	   = (int8_t)s;
		if(v == 0)
			x86::xor_rr(code(), p, p); // x0 is always zero
		else
			x86::mov_mr(code(), p, x86::REG_REGS, (int32_t)(v * 8));
		return p;
	}

	uint8_t JIT_Emitter::hreg_for_write(uint32_t v, uint32_t keep1, uint32_t keep2)
	{
		VReg& g = vr[v];
		if(g.slot >= 0)
			return phys((uint8_t)g.slot);
		uint8_t s  = free_slot(keep1, keep2);
		uint8_t p  = phys(s);
		hr_vreg[s] = (uint8_t)v;
		g.slot	   = (int8_t)s;
		return p;
	}

	void JIT_Emitter::flush_guest(uint32_t v)
	{
		VReg& g = vr[v];
		if(g.slot >= 0 && g.dirty)
		{
			x86::mov_rm(code(), x86::REG_REGS, (int32_t)(v * 8), phys((uint8_t)g.slot));
			g.dirty = false;
		}
	}

	void JIT_Emitter::flush_all()
	{
		for(uint32_t v = 1; v < 32; v++)
			flush_guest(v);
	}

	void JIT_Emitter::load_imm64(uint8_t p, uint64_t imm)
	{
		x86::mov_imm64(code(), p, imm);
	}

	void JIT_Emitter::emit_prologue()
	{
		// rbp frame keeps our pushes out of the caller's red zone. r15 is
		// pushed alongside the other callee-saved regs: the exit tail loads
		// it with the private link slot address (and the chain dispatcher
		// refills through it), so every return path must pop it back.
		x86::push_r(code(), x86::REG_RBP);
		x86::mov_rr(code(), x86::REG_RBP, x86::REG_RSP);
		x86::push_r(code(), x86::REG_R15);
		x86::push_r(code(), x86::REG_R13);
		x86::push_r(code(), x86::REG_R12);
		x86::mov_rr(code(), x86::REG_R12, x86::REG_RDI);
		x86::mov_mr(code(), x86::REG_R13, x86::REG_R12, x86::CTX_OFF_REGS);
		blk->chain_off = code().pos; // chain entry: assumes R12/R13 already set
	}

	void JIT_Emitter::emit_epilogue(uint32_t guest_bytes, uint32_t guest_count)
	{
		// Only straight-line blocks reach here; control transfers emit their
		// own exits (em.exited = true).
		if(exited)
			__builtin_trap();
		emit_block_exit(guest_bytes, guest_count);
	}

	bool JIT_Emitter::eof() const
	{
		// The decode loop must leave room for the epilogue, every miss stub,
		// and a margin that absorbs the emission overshoot of the last
		// instruction (the check runs before each instruction, so the buffer
		// can still grow by one instruction past this point). With STUB_BYTES
		// and EPILOGUE_BYTES as true upper bounds the epilogue + stubs then
		// always fit inside RVJIT_FUNC_SIZE.
		return code().pos + EPILOGUE_BYTES + RVJIT_FUNC_MARGIN + stub_reserve >= RVJIT_FUNC_SIZE;
	}

	void JIT_Emitter::release_slot(uint8_t slot)
	{
		hr_vreg[slot] = 0xFF;
	}

	void JIT_Emitter::emit_load(uint32_t rd, uint32_t rs1, int64_t imm, uint8_t width, bool sign_extend)
	{
		x86::CodeBuf& cb = code();
		// No flush here: the miss stub for this instruction commits the dirty
		// registers before the interpreter resumes.
		const uint32_t instr = blk->instr_index;

		uint8_t H = 0xFF;
		if(rd == 0)
		{
			// Preserve fault semantics: translate/check, then discard.
			uint8_t S1 = hreg_for_read(rs1);
			uint8_t s  = free_slot(rs1);
			H		   = phys(s);
			x86::lea_r64_mem(cb, H, S1, (int32_t)imm);
			emit_tlb_checks(*this, H, width, false, instr);
			release_slot(s);
			return;
		}

		uint8_t S1 = hreg_for_read(rs1);
		flush_guest(rd);
		uint8_t D  = hreg_for_write(rd, rs1);
		x86::lea_r64_mem(cb, D, S1, (int32_t)imm);
		H = D;
		emit_tlb_checks(*this, H, width, false, instr);

		switch(width)
		{
			case 1:
				if(sign_extend)
					x86::movsx_r64_m8(cb, D, D, 0);
				else
					x86::movzx_r64_m8(cb, D, D, 0);
				break;
			case 2:
				if(sign_extend)
					x86::movsx_r64_m16(cb, D, D, 0);
				else
					x86::movzx_r64_m16(cb, D, D, 0);
				break;
			case 4:
				if(sign_extend)
					x86::movsxd_r64_m32(cb, D, D, 0);
				else
					x86::mov_r32_m32(cb, D, D, 0);
				break;
			default:
				x86::mov_mr(cb, D, D, 0);
				break;
		}
		vr[rd].dirty = true;
	}

	void JIT_Emitter::emit_store(uint32_t rs1, int64_t imm, uint32_t rs2, uint8_t width)
	{
		x86::CodeBuf& cb = code();
		// See emit_load().
		const uint32_t instr = blk->instr_index;

		// Load the value first, then protect both operands while the address
		// scratch is allocated.
		uint8_t V  = hreg_for_read(rs2);
		uint8_t S1 = hreg_for_read(rs1, rs2);
		uint8_t s  = free_slot(rs1, rs2);
		uint8_t H  = phys(s);
		x86::lea_r64_mem(cb, H, S1, (int32_t)imm);
		emit_tlb_checks(*this, H, width, true, instr);

		switch(width)
		{
			case 1:
				x86::mov_m8_r8(cb, H, 0, V);
				break;
			case 2:
				x86::mov_m16_r16(cb, H, 0, V);
				break;
			case 4:
				x86::mov_m32_r32(cb, H, 0, V);
				break;
			default:
				x86::mov_rm(cb, H, 0, V);
				break;
		}
		release_slot(s);
	}

	void JIT_Emitter::emit_miss_stubs()
	{
		x86::CodeBuf& cb = code();
		size_t i		 = 0;
		while(i < misses.size())
		{
			const uint32_t instr  = misses[i].instr;
			const uint32_t bytes  = misses[i].bytes;
			const uint8_t  dcnt   = misses[i].dcnt;
			uint8_t dv[6], ds[6];
			for(uint8_t k = 0; k < dcnt; k++)
			{
				dv[k] = misses[i].dv[k];
				ds[k] = misses[i].ds[k];
			}
			const uint32_t target = cb.pos;
			while(i < misses.size() && misses[i].instr == instr)
			{
				x86::patch_rel32(cb, misses[i].rel_pos, target);
				i++;
			}
			// Commit the dirty cached registers, then exit with the pc/count
			// captured before the faulting instruction. A TLB miss is a yield
			// to C++ (softmmu refill), not a chainable handoff: account its
			// instructions into the chain budget and return via the single
			// C++ frame, which the chain tail must not bypass.
			for(uint8_t k = 0; k < dcnt; k++)
				x86::mov_rm(cb, x86::REG_REGS, (int32_t)(dv[k] * 8), phys(ds[k]));
			x86::mov_mr(cb, x86::REG_RAX, x86::REG_CTX, x86::CTX_OFF_ENTRY);
			x86::add_imm(cb, x86::REG_RAX, (int32_t)bytes);
			x86::mov_rm(cb, x86::REG_CTX, x86::CTX_OFF_EXIT, x86::REG_RAX);
			x86::mov_m64_imm(cb, x86::REG_CTX, x86::CTX_OFF_EXIT_COUNT, (int32_t)instr);
			x86::mov_mr(cb, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_CHAIN_BUDGET);
			x86::arith_rm64(cb, 5, x86::REG_RDX, x86::REG_CTX, x86::CTX_OFF_EXIT_COUNT);
			x86::mov_rm(cb, x86::REG_CTX, x86::CTX_OFF_CHAIN_BUDGET, x86::REG_RDX);
			x86::pop_r(cb, x86::REG_R12);
			x86::pop_r(cb, x86::REG_R13);
			x86::pop_r(cb, x86::REG_R15);
			x86::pop_r(cb, x86::REG_RBP);
			x86::ret(cb);
		}
	}

	void JIT_Emitter::push_miss(uint32_t rel_pos, uint32_t instr)
	{
		if(misses.empty() || misses.back().instr != instr)
			stub_reserve += STUB_BYTES;
		MissSite s;
		s.rel_pos = rel_pos;
		s.instr	  = instr;
		s.bytes	  = blk->instr_bytes;
		s.dcnt	  = snapshot_dirty(s.dv, s.ds);
		misses.push_back(s);
	}

	uint8_t JIT_Emitter::snapshot_dirty(uint8_t* dv, uint8_t* ds)
	{
		uint8_t n = 0;
		for(uint8_t v = 1; v < 32 && n < 6; v++)
		{
			if(vr[v].dirty && vr[v].slot >= 0)
			{
				dv[n] = v;
				ds[n] = (uint8_t)vr[v].slot;
				n++;
			}
		}
		return n;
	}

	void JIT_Emitter::emit_block_exit(uint32_t delta_va, uint32_t count)
	{
		x86::CodeBuf& cb = code();
		flush_all();
		// exit_pc = entry_pc + delta_va (VA), accounting for mixed 2/4-byte
		// compressed + uncompressed instructions inside one block.
		x86::mov_mr(cb, x86::REG_RAX, x86::REG_CTX, x86::CTX_OFF_ENTRY);
		x86::add_imm(cb, x86::REG_RAX, (int32_t)delta_va);
		emit_chain_tail(*this, count);
	}

	void JIT_Emitter::emit_block_exit_rax(uint32_t count)
	{
		x86::CodeBuf& cb = code();
		flush_all();
		emit_chain_tail(*this, count);
	}

	void JIT_Emitter::emit_link_stubs()
	{
		x86::CodeBuf& cb = code();
		// Shared fallback: hop to the chain dispatcher. r15 already holds this
		// block's slot base, so a dispatcher hit re-stamps the right private
		// slot (and a miss pops the block frame and returns to the runner).
		const uint32_t go_pos = cb.pos;
		x86::mov_imm64(cb, x86::REG_R11, x86::chain_dispatcher());
		x86::jmp_r(cb, x86::REG_R11);

		// Shared return stub: the chain budget is exhausted (or a guard
		// failed and the dispatcher bounced); the exit already wrote
		// exit_pc/exit_count, so pop the block frame and return to the runner.
		const uint32_t ret_pos = cb.pos;
		x86::pop_r(cb, x86::REG_R12);
		x86::pop_r(cb, x86::REG_R13);
		x86::pop_r(cb, x86::REG_R15);
		x86::pop_r(cb, x86::REG_RBP);
		x86::ret(cb);

		for(uint32_t i = 0; i < blk->n_exits; i++)
		{
			x86::patch_rel32(cb, blk->exits[i].budget_js, ret_pos);
			for(int k = 0; k < 6; k++)
				x86::patch_rel32(cb, blk->exits[i].fail[k], go_pos);
		}
	}

	void JIT_Emitter::emit_cond_exit(uint32_t rs1, uint32_t rs2, uint8_t cc, int64_t imm,
									 uint32_t off, uint32_t size, uint32_t count_before, bool check_align)
	{
		x86::CodeBuf& cb = code();
		uint8_t S1		 = hreg_for_read(rs1);
		uint8_t S2		 = (rs2 == rs1) ? S1 : hreg_for_read(rs2, rs1);
		// Flush dirty regs before the branch: the not-taken exit's flush would
		// otherwise clear them before the taken exit runs, losing the updates.
		flush_all();
		x86::cmp_rr(cb, S1, S2);
		const uint32_t taken_rel = x86::jcc32(cb, cc);
		emit_block_exit(off + size, count_before + 1);
		const uint32_t taken_target = cb.pos;
		x86::mov_mr(cb, x86::REG_RAX, x86::REG_CTX, x86::CTX_OFF_ENTRY);
		x86::add_imm(cb, x86::REG_RAX, (int32_t)((int64_t)off + imm));
		if(check_align)
		{
			x86::test_imm(cb, x86::REG_RAX, 1);
			push_miss(x86::jcc32(cb, CC_JNE), count_before);
		}
		emit_block_exit_rax(count_before + 1);
		x86::patch_rel32(cb, taken_rel, taken_target);
		exited = true;
	}

void JIT_Emitter::emit_cond_exit_zero(uint32_t rs, uint8_t cc, int64_t imm,
									  uint32_t off, uint32_t count_before)
	{
		x86::CodeBuf& cb = code();
		uint8_t S		 = hreg_for_read(rs);
		flush_all();
		x86::test_rr(cb, S, S);
		const uint32_t taken_rel = x86::jcc32(cb, cc);
		emit_block_exit(off + 2, count_before + 1);
		const uint32_t taken_target = cb.pos;
		x86::mov_mr(cb, x86::REG_RAX, x86::REG_CTX, x86::CTX_OFF_ENTRY);
		x86::add_imm(cb, x86::REG_RAX, (int32_t)((int64_t)off + imm));
		emit_block_exit_rax(count_before + 1);
		x86::patch_rel32(cb, taken_rel, taken_target);
		exited = true;
	}

	void JIT_Emitter::emit_jump(uint32_t link_rd, uint32_t src_rs1, int64_t imm,
								uint32_t off, uint32_t size, uint32_t count_before,
								bool check_align, bool from_rs1)
	{
		x86::CodeBuf& cb = code();
		// Flush the block's state; the target block reloads regs from memory.
		flush_all();
		if(from_rs1)
		{
			uint8_t S1 = hreg_for_read(src_rs1);
			x86::mov_rr(cb, x86::REG_RAX, S1);
			if(imm != 0)
				x86::add_imm(cb, x86::REG_RAX, (int32_t)imm);
			x86::and_imm(cb, x86::REG_RAX, -2); // target & ~1
		}
		else
		{
			x86::mov_mr(cb, x86::REG_RAX, x86::REG_CTX, x86::CTX_OFF_ENTRY);
			x86::add_imm(cb, x86::REG_RAX, (int32_t)((int64_t)off + imm));
		}
		if(check_align)
		{
			x86::test_imm(cb, x86::REG_RAX, 1);
			push_miss(x86::jcc32(cb, CC_JNE), count_before);
		}
		if(link_rd != 0)
		{
			// link = entry_pc + off + size; the exit flush commits it.
			uint8_t D = hreg_for_write(link_rd, src_rs1);
			x86::mov_mr(cb, D, x86::REG_CTX, x86::CTX_OFF_ENTRY);
			x86::add_imm(cb, D, (int32_t)((int64_t)off + size));
			vr[link_rd].dirty = true;
		}
		emit_block_exit_rax(count_before + 1);
		exited = true;
	}

	void JIT_Emitter::emit_r_to(uint8_t dstReg, uint8_t src1Reg, uint8_t src2Reg, ALUOp op, bool wVariant)
	{
		const bool r1	   = (src1Reg != 0);
		const bool r2	   = (src2Reg != 0);
		const bool shiftOp = (op == ALUOp::SLL || op == ALUOp::SRL || op == ALUOp::SRA);
		uint8_t S1 = 0xFF, S2 = 0xFF;
		if(r1)
			S1 = hreg_for_read(src1Reg);
		if(r2)
			S2 = (src2Reg == src1Reg) ? S1 : hreg_for_read(src2Reg, src1Reg);
		uint8_t D = hreg_for_write(dstReg, src1Reg, src2Reg);

		switch(op)
		{
			case ALUOp::ADD:
				if(!r1 && !r2)
				{
					x86::xor_rr(code(), D, D);
				}
				else if(!r1)
				{
					if(D != S2) x86::mov_rr(code(), D, S2);
				}
				else if(!r2)
				{
					if(D != S1) x86::mov_rr(code(), D, S1);
				}
				else if(D == S1)
				{
					x86::add_rr(code(), D, S2);
				}
				else if(D == S2)
				{
					x86::add_rr(code(), D, S1);
				}
				else
				{
					x86::mov_rr(code(), D, S1);
					x86::add_rr(code(), D, S2);
				}
				break;
			case ALUOp::SUB: // dst = rs1 - rs2 (non commutative)
				if(!r1 && !r2)
				{
					x86::xor_rr(code(), D, D);
				}
				else if(!r2)
				{
					if(D != S1) x86::mov_rr(code(), D, S1);
				}
				else if(!r1)
				{
					// dst = 0 - rs2
					if(D == S2)
					{
						x86::mov_rr(code(), x86::REG_TMP, S2);
						x86::neg64(code(), x86::REG_TMP);
						x86::mov_rr(code(), D, x86::REG_TMP);
					}
					else
					{
						x86::xor_rr(code(), D, D);
						x86::sub_rr(code(), D, S2);
					}
				}
				else if(D == S1)
				{
					x86::sub_rr(code(), D, S2);
				}
				else if(D == S2)
				{
					// dst slot aliases rs2; compute in scratch first.
					x86::mov_rr(code(), x86::REG_TMP, S1);
					x86::sub_rr(code(), x86::REG_TMP, S2);
					x86::mov_rr(code(), D, x86::REG_TMP);
				}
				else
				{
					x86::mov_rr(code(), D, S1);
					x86::sub_rr(code(), D, S2);
				}
				break;
			case ALUOp::XOR:
			case ALUOp::OR:
			case ALUOp::AND:
			{
				if(!r1 && !r2)
				{
					x86::xor_rr(code(), D, D);
				}
				else if(!r1)
				{
					if(op == ALUOp::AND)
						x86::xor_rr(code(), D, D); // 0 & x == 0
					else
					{
						if(D != S2) x86::mov_rr(code(), D, S2);
					}
				}
				else if(!r2)
				{
					if(op == ALUOp::AND)
						x86::xor_rr(code(), D, D);
					else
					{
						if(D != S1) x86::mov_rr(code(), D, S1);
					}
				}
				else if(D == S1)
				{
					if(op == ALUOp::XOR)
						x86::xor_rr(code(), D, S2);
					else if(op == ALUOp::OR)
						x86::or_rr(code(), D, S2);
					else
						x86::and_rr(code(), D, S2);
				}
				else if(D == S2)
				{
					if(op == ALUOp::XOR)
						x86::xor_rr(code(), D, S1);
					else if(op == ALUOp::OR)
						x86::or_rr(code(), D, S1);
					else
						x86::and_rr(code(), D, S1);
				}
				else
				{
					x86::mov_rr(code(), D, S1);
					if(op == ALUOp::XOR)
						x86::xor_rr(code(), D, S2);
					else if(op == ALUOp::OR)
						x86::or_rr(code(), D, S2);
					else
						x86::and_rr(code(), D, S2);
				}
				break;
			}
			case ALUOp::SLL:
			case ALUOp::SRL:
			case ALUOp::SRA:
			{
				const uint8_t fields = (op == ALUOp::SLL) ? 4 : (op == ALUOp::SRL) ? 5
																				   : 7;
				if(!r1 && !r2)
				{
					x86::xor_rr(code(), D, D);
				}
				else if(!r2)
				{
					if(D != S1) x86::mov_rr(code(), D, S1);
				}
				else if(!r1)
				{
					x86::xor_rr(code(), D, D);
				}
				else
				{
					// D may alias S2 (rd == rs2): save the count before S1 clobbers it.
					x86::mov_rr(code(), x86::REG_TMP, S2);
					if(D != S1) x86::mov_rr(code(), D, S1);
					if(wVariant)
						x86::shift_r32_cl(code(), fields, D);
					else
						x86::shift_r64_cl(code(), fields, D);
				}
				break;
			}
			case ALUOp::SLT:
			case ALUOp::SLTU:
			{
				const uint8_t cc = (op == ALUOp::SLT) ? 0x9C : 0x92;
				if(!r1 && !r2)
				{
					x86::xor_rr(code(), D, D);
				}
				else if(!r1)
				{
					if(D == S2)
					{
						x86::xor_rr(code(), x86::REG_TMP, x86::REG_TMP);
						x86::cmp_rr(code(), x86::REG_TMP, S2);
						x86::setcc(code(), cc, x86::REG_TMP);
						x86::movzx_r64_r8(code(), x86::REG_TMP, x86::REG_TMP);
						x86::mov_rr(code(), D, x86::REG_TMP);
					}
					else
					{
						x86::xor_rr(code(), D, D);
						x86::cmp_rr(code(), D, S2);
						x86::setcc(code(), cc, D);
						x86::movzx_r64_r8(code(), D, D);
					}
				}
				else if(!r2)
				{
					if(D == S1)
					{
						x86::xor_rr(code(), x86::REG_TMP, x86::REG_TMP);
						x86::cmp_rr(code(), S1, x86::REG_TMP);
						x86::setcc(code(), cc, x86::REG_TMP);
						x86::movzx_r64_r8(code(), x86::REG_TMP, x86::REG_TMP);
						x86::mov_rr(code(), D, x86::REG_TMP);
					}
					else
					{
						x86::xor_rr(code(), D, D);
						x86::cmp_rr(code(), S1, D);
						x86::setcc(code(), cc, D);
						x86::movzx_r64_r8(code(), D, D);
					}
				}
				else
				{
					x86::cmp_rr(code(), S1, S2);
					x86::setcc(code(), cc, D);
					x86::movzx_r64_r8(code(), D, D);
				}
				break;
			}
		}
		vr[dstReg].dirty = true;
		if(wVariant)
			x86::movsxd(code(), D, D);
	}

	void JIT_Emitter::emit_m_r_to(uint8_t dstReg, uint8_t src1Reg, uint8_t src2Reg, MOp op, bool wVariant)
	{
		const bool r1 = (src1Reg != 0), r2 = (src2Reg != 0);
		const bool mulF	   = (op == MOp::MUL || op == MOp::MULH || op == MOp::MULHU || op == MOp::MULHSU);
		const bool rem	   = (op == MOp::REM || op == MOp::REMU);
		const bool signed_ = (op == MOp::MULH || op == MOp::MULHSU || op == MOp::DIV || op == MOp::REM);
		x86::CodeBuf& cb   = code();

		// x0 shortcuts: rs2 == 0 wins over rs1 == 0 (matches the interpreter's
		// DIV-by-zero precedence). The mul/dividend zero cases both yield 0.
		if(!r2)
		{
			if(mulF)
			{
				uint8_t D = hreg_for_write(dstReg, src1Reg, 0xFFFFFFFFu);
				x86::xor_rr(cb, D, D);
			}
			else if(!rem)
			{
				// DIV by zero: quotient = all ones.
				uint8_t D = hreg_for_write(dstReg, src1Reg, 0xFFFFFFFFu);
				x86::mov_imm32(cb, D, -1);
			}
			else
			{
				// REM by zero: remainder = dividend (REMW: sext(int32 rs1)).
				uint8_t S1 = hreg_for_read(src1Reg);
				uint8_t D  = hreg_for_write(dstReg, src1Reg, 0xFFFFFFFFu);
				if(op == MOp::REM && wVariant)
				{
					if(D != S1) x86::mov_rr32(cb, D, S1);
					x86::movsxd(cb, D, D);
				}
				else if(D != S1)
				{
					x86::mov_rr(cb, D, S1);
				}
			}
			vr[dstReg].dirty = true;
			return;
		}
		if(!r1 && (mulF || rem))
		{
			// 0 * y = 0 and 0 % y = 0 regardless of y's value (y==0 yields
			// dividend==0, still 0). DIV needs the runtime divisor check, so
			// it falls through to the general path with a zero dividend.
			uint8_t D = hreg_for_write(dstReg, src1Reg, 0xFFFFFFFFu);
			x86::xor_rr(cb, D, D);
			vr[dstReg].dirty = true;
			return;
		}

		// Both operands live: load them first, then allocate the destination
		// with both sources protected (S1/S2 slots stay valid throughout).
		uint8_t S1 = hreg_for_read(src1Reg);
		uint8_t S2 = (src2Reg == src1Reg) ? S1 : hreg_for_read(src2Reg, src1Reg);
		uint8_t D  = hreg_for_write(dstReg, src1Reg, src2Reg);

		if(op == MOp::MUL)
		{
			// Low half of the product (MUL is commutative, so the aliasing
			// shifts the multiply to whichever operand's slot D shares).
			if(D == S1)
			{
				if(wVariant)
				{
					x86::imul_rr32(cb, D, S2);
					x86::movsxd(cb, D, D);
				}
				else
					x86::imul_rr(cb, D, S2);
			}
			else if(D == S2)
			{
				if(wVariant)
				{
					x86::imul_rr32(cb, D, S1);
					x86::movsxd(cb, D, D);
				}
				else
					x86::imul_rr(cb, D, S1);
			}
			else
			{
				if(wVariant)
				{
					if(D != S1) x86::mov_rr32(cb, D, S1);
					x86::imul_rr32(cb, D, S2);
					x86::movsxd(cb, D, D);
				}
				else
				{
					if(D != S1) x86::mov_rr(cb, D, S1);
					x86::imul_rr(cb, D, S2);
				}
			}
			vr[dstReg].dirty = true;
			return;
		}

		if(mulF)
		{
			// High half via RDX:RAX. S1 is never RAX/RDX/RCX (pool excludes
			// them), so the moves below cannot clobber a live operand.
			x86::mov_rr(cb, x86::REG_ACC0, S1);
			if(op == MOp::MULHSU)
			{
				x86::mul_r(cb, S2); // unsigned product; high in RDX
				// signed(x)*unsigned(y) high = high_un - (x<0 ? y : 0)
				x86::mov_rr(cb, x86::REG_TMP, S1);
				x86::shift_r64_imm(cb, 7, x86::REG_TMP, 63);
				x86::and_rr(cb, x86::REG_TMP, S2);
				x86::sub_rr(cb, x86::REG_ACC1, x86::REG_TMP);
			}
			else
			{
				if(op == MOp::MULH)
					x86::imul_r(cb, S2); // signed 128-bit
				else
					x86::mul_r(cb, S2); // unsigned 128-bit
			}
			if(D != x86::REG_ACC1)
				x86::mov_rr(cb, D, x86::REG_ACC1);
			vr[dstReg].dirty = true;
			return;
		}

		// Division / remainder. RAX = dividend, RDX = hi dividend, RCX = divisor.
		if(wVariant)
		{
			if(signed_)
			{
				x86::movsxd(cb, x86::REG_TMP, S2);
				x86::movsxd(cb, x86::REG_ACC0, S1);
			}
			else
			{
				x86::mov_rr32(cb, x86::REG_TMP, S2);
				x86::mov_rr32(cb, x86::REG_ACC0, S1);
			}
		}
		else
		{
			x86::mov_rr(cb, x86::REG_TMP, S2);
			x86::mov_rr(cb, x86::REG_ACC0, S1);
		}

		// RISC-V DIV/REM by zero never faults: quotient = -1, remainder =
		// dividend. x86 DIV/IDIV would raise #DE, so guard explicitly.
		x86::or_rr(cb, x86::REG_TMP, x86::REG_TMP);
		uint32_t fix_nz = x86::jcc8(cb, 0x75); // jne -> real division
		if(rem)
		{
			if(op == MOp::REMU && wVariant)
			{
				// REMUW by zero returns the full rs1 value.
				if(D != S1) x86::mov_rr(cb, D, S1);
			}
			else if(op == MOp::REM && wVariant)
			{
				x86::mov_rr(cb, D, x86::REG_ACC0); // already sext(int32 rs1)
			}
			else if(D != x86::REG_ACC0)
			{
				x86::mov_rr(cb, D, x86::REG_ACC0);
			}
		}
		else
		{
			x86::mov_imm32(cb, D, -1);
		}
		uint32_t fix_done0 = x86::jcc8(cb, 0xEB); // jmp -> done

		const uint32_t label_div = cb.pos;
		x86::patch_rel8(cb, fix_nz, label_div);

		if(signed_)
		{
			// IDIV faults on MIN / -1 too; handle that pair separately.
			x86::cmp_imm(cb, x86::REG_TMP, -1);
			uint32_t fix_n1 = x86::jcc8(cb, 0x75); // jne -> idiv
			if(rem)
			{
				x86::xor_rr(cb, D, D);
			}
			else
			{
				if(wVariant)
				{
					x86::neg32(cb, x86::REG_ACC0);
					x86::movsxd(cb, x86::REG_ACC0, x86::REG_ACC0);
				}
				else
				{
					x86::neg64(cb, x86::REG_ACC0);
				}
				if(D != x86::REG_ACC0)
					x86::mov_rr(cb, D, x86::REG_ACC0);
			}
			uint32_t fix_done1		  = x86::jcc8(cb, 0xEB);
			const uint32_t label_idiv = cb.pos;
			x86::patch_rel8(cb, fix_n1, label_idiv);

			if(wVariant)
			{
				x86::cdq(cb);
				x86::idiv_r32(cb, x86::REG_TMP);
			}
			else
			{
				x86::cqo(cb);
				x86::idiv_r(cb, x86::REG_TMP);
			}
			if(rem)
			{
				if(wVariant)
				{
					x86::mov_rr32(cb, D, x86::REG_ACC1);
					x86::movsxd(cb, D, D);
				}
				else
				{
					if(D != x86::REG_ACC1) x86::mov_rr(cb, D, x86::REG_ACC1);
				}
			}
			else
			{
				if(wVariant)
				{
					x86::mov_rr32(cb, D, x86::REG_ACC0);
					x86::movsxd(cb, D, D);
				}
				else
				{
					if(D != x86::REG_ACC0) x86::mov_rr(cb, D, x86::REG_ACC0);
				}
			}
			x86::patch_rel8(cb, fix_done1, cb.pos);
			x86::patch_rel8(cb, fix_done0, cb.pos);
		}
		else
		{
			x86::xor_rr(cb, x86::REG_ACC1, x86::REG_ACC1);
			if(wVariant)
			{
				x86::div_r32(cb, x86::REG_TMP);
				if(rem)
				{
					x86::mov_rr32(cb, D, x86::REG_ACC1);
					x86::movsxd(cb, D, D);
				}
				else
				{
					x86::mov_rr32(cb, D, x86::REG_ACC0);
					x86::movsxd(cb, D, D);
				}
			}
			else
			{
				x86::div_r(cb, x86::REG_TMP);
				if(rem)
				{
					if(D != x86::REG_ACC1) x86::mov_rr(cb, D, x86::REG_ACC1);
				}
				else
				{
					if(D != x86::REG_ACC0) x86::mov_rr(cb, D, x86::REG_ACC0);
				}
			}
			x86::patch_rel8(cb, fix_done0, cb.pos);
		}
		vr[dstReg].dirty = true;
	}

	void JIT_Emitter::emit_i_to(uint8_t dstReg, uint8_t src1Reg, int64_t imm, ALUOp op, bool wVariant)
	{
		const bool r1 = (src1Reg != 0);
		uint8_t S1	  = r1 ? hreg_for_read(src1Reg) : 0xFF;
		uint8_t D	  = hreg_for_write(dstReg, src1Reg, 0xFFFFFFFFu);
		switch(op)
		{
			case ALUOp::ADD:
				if(!r1)
					x86::mov_imm32(code(), D, (int32_t)imm);
				else if(D == S1)
					x86::add_imm(code(), D, (int32_t)imm);
				else
				{
					x86::mov_rr(code(), D, S1);
					x86::add_imm(code(), D, (int32_t)imm);
				}
				break;
			case ALUOp::XOR:
			case ALUOp::OR:
			case ALUOp::AND:
				if(!r1)
				{
					if(op == ALUOp::AND)
						x86::xor_rr(code(), D, D);
					else
						x86::mov_imm32(code(), D, (int32_t)imm);
				}
				else if(D == S1)
				{
					if(op == ALUOp::XOR)
						x86::xor_imm(code(), D, (int32_t)imm);
					else if(op == ALUOp::OR)
						x86::or_imm(code(), D, (int32_t)imm);
					else
						x86::and_imm(code(), D, (int32_t)imm);
				}
				else
				{
					x86::mov_rr(code(), D, S1);
					if(op == ALUOp::XOR)
						x86::xor_imm(code(), D, (int32_t)imm);
					else if(op == ALUOp::OR)
						x86::or_imm(code(), D, (int32_t)imm);
					else
						x86::and_imm(code(), D, (int32_t)imm);
				}
				break;
			case ALUOp::SLL:
			case ALUOp::SRL:
			case ALUOp::SRA:
			{
				// shift by immediate (caller guarantees 0 <= imm < 64)
				uint8_t shamt = (uint8_t)(imm & 0x3F);
				if(!r1)
					x86::xor_rr(code(), D, D);
				else
				{
					if(D != S1)
						x86::mov_rr(code(), D, S1);
					if(op == ALUOp::SLL)
					{
						if(wVariant)
							x86::shift_r32_imm(code(), 4, D, shamt);
						else
							x86::shift_r64_imm(code(), 4, D, shamt);
					}
					else if(op == ALUOp::SRL)
					{
						if(wVariant)
							x86::shift_r32_imm(code(), 5, D, shamt);
						else
							x86::shift_r64_imm(code(), 5, D, shamt);
					}
					else
					{
						if(wVariant)
							x86::shift_r32_imm(code(), 7, D, shamt);
						else
							x86::shift_r64_imm(code(), 7, D, shamt);
					}
				}
				break;
			}
			case ALUOp::SLT:
			case ALUOp::SLTU:
			{
				const uint8_t cc = (op == ALUOp::SLT) ? 0x9C : 0x92;
				if(!r1 && imm == 0)
				{
					x86::xor_rr(code(), D, D);
				}
				else if(!r1)
				{
					x86::xor_rr(code(), D, D);
					x86::cmp_imm(code(), D, (int32_t)imm);
					x86::setcc(code(), cc, D);
					x86::movzx_r64_r8(code(), D, D);
				}
				else
				{
					x86::cmp_imm(code(), S1, (int32_t)imm);
					x86::setcc(code(), cc, D);
					x86::movzx_r64_r8(code(), D, D);
				}
				break;
			}
		}
		vr[dstReg].dirty = true;
		if(wVariant)
			x86::movsxd(code(), D, D);
	}
	void JIT_Emitter::emit_u_to(uint8_t dstReg, int32_t imm, ALUOp op, uint64_t pc)
	{
		uint8_t D = hreg_for_write(dstReg);
		switch(op)
		{
			case ALUOp::LUI:
				x86::mov_imm64(code(), D, imm);
				break;
			case ALUOp::AUIPC:
				x86::mov_imm64(code(), D, imm + pc);
				break;
		}
		vr[dstReg].dirty = true;
	}
}
