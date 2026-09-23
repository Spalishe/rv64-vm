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

/*
 * Block buffer + virtual/host register allocator for the RV64I translators.
 *
 * Guest registers live in memory at &hart.GPR[0]; the allocator caches a
 * window of them in host registers and flushes dirty values at the end of
 * the block. Interface only; the x86-64 implementation lives in
 * src/jit/arch/x86_64/emit.cpp.
 */
#include "rvjit_x86_64.hpp"
#include <cstdint>
#include <vector>

namespace rv64vm::jit
{
	struct JIT_Block
	{
		x86::CodeBuf code;
		uint64_t start_phys	 = 0;
		uint32_t count		 = 0;
		uint32_t bytes_guest = 0;
		uint32_t chain_off	 = 0; // bytes of emit_prologue(); the chain entry target
		uint64_t smc_epoch	 = 0;
		uint32_t instr_index = 0; // index of the instruction being compiled
		uint32_t instr_bytes = 0; // guest bytes before the instruction being compiled
		bool valid			 = false;

		// Per-exit chain-link sites (one shared table per block, filled by
		// emit_block_exit / emit_block_exit_rax through the emitter). Every
		// normal block exit emits a self-contained tail that validates its
		// private link slot (data_idx) before hopping; on staleness it falls
		// back to the shared chain dispatcher, which re-stamps the slot on
		// hit. lea_disp_off/budget_js/fail[5] are code-buffer positions whose
		// rel32 targets are patched by emit_link_stubs(); lea_disp_off is the
		// slot of the lea's disp32 field, relocated (RIP-relative) by
		// compile() once the arena address is known.
		struct ExitLink
		{
			uint32_t data_idx;	  // index into this block's private slot region
			uint32_t lea_disp_off; // disp32 field of "lea r15, [rip+slot]"
			uint32_t budget_js;	  // js (budget exhausted) -> block return stub
			uint32_t fail[6];	  // guards that miss -> shared chain dispatcher
		};
		ExitLink exits[6];
		uint32_t n_exits = 0;
	};

	struct VReg
	{
		int8_t slot = -1; // -1 = not cached
		bool dirty	= false;
	};

	enum class ALUOp
	{
		ADD,
		SUB,
		XOR,
		OR,
		AND,
		SLL,
		SRL,
		SRA,
		SLT,
		SLTU,
		LUI,
		AUIPC
	};

	// RV64M register-form operations (MULW/DIVW/etc. are the wVariant).
	enum class MOp
	{
		MUL,
		MULH,
		MULHU,
		MULHSU,
		DIV,
		DIVU,
		REM,
		REMU
	};

	/*
	 * Block emitter / register allocator.
	 *
	 * Operands are always loaded into host registers BEFORE the destination
	 * slot is allocated, and free_slot() must not evict those live operands;
	 * otherwise an ALU op silently computes on a stale register. The R-type
	 * shifts take the variable count from src2Reg; the I-type shifts from an
	 * immediate in emit_i_to.
	 *
	 * RAX/RDX/RCX are dedicated scratch (never in the allocator pool); the
	 * M-extension ops use RAX/RDX as the multiply/divide accumulator pair.
	 */
	struct JIT_Emitter
	{
		JIT_Block* blk;
		VReg vr[32];
		uint8_t hr_vreg[x86::RVJIT_HOST_REGS]; // 0xFF = free

		// Translation context baked into the block at compile time. The block
		// is only dispatched while the hart matches these, so the emitted
		// permission masks stay valid. ASID is deliberately NOT baked: the
		// inline TLB check validates it at runtime (asid match or global),
		// and blocks never cross a page, so the phys key alone identifies
		// the instruction stream for every address space (see lookup()).
		uint8_t eff_mode = 0; // Hart::PrivilegeMode as int (0=U,1=S,3=M)
		bool mxr		 = false;
		bool sum		 = false;

		// TLB-miss / branch-misalign fixups: each [rel_pos] is a rel32 jcc
		// into the stub of [instr], patched by emit_miss_stubs(). Dirty guest
		// registers + their host slots are snapshotted at this point so the stub
		// can commit them before the block exits to the interpreter.
		struct MissSite
		{
			uint32_t rel_pos;
			uint32_t instr; // instruction index (matches JIT_Block::instr_index)
			uint32_t bytes; // guest bytes executed before this instruction
			uint8_t dcnt;	// number of dirty registers at this point (<= 6)
			uint8_t dv[6];	// them: guest register numbers
			uint8_t ds[6];	// them: host pool slots holding the values
		};
		std::vector<MissSite> misses;
		uint32_t stub_reserve = 0; // bytes reserved for the future miss stubs

		// A control-transfer translator already emitted its own exit, so the
		// default emit_epilogue() must be skipped.
		bool exited = false;

		JIT_Emitter(JIT_Block* b);

		x86::CodeBuf& code() const;

		// Never evicts slots holding keep1/keep2 (may be live operands).
		uint8_t free_slot(uint32_t keep1 = 0xFFFFFFFFu, uint32_t keep2 = 0xFFFFFFFFu);
		// Releases a slot previously handed out by free_slot() (hr_vreg == 0xFE).
		void release_slot(uint8_t slot);

		uint8_t phys(uint8_t slot) const;

		uint8_t hreg_for_read(uint32_t v, uint32_t keep = 0xFFFFFFFFu);

		uint8_t hreg_for_write(uint32_t v, uint32_t keep1 = 0xFFFFFFFFu, uint32_t keep2 = 0xFFFFFFFFu);

		void flush_guest(uint32_t v);

		void flush_all();

		void load_imm64(uint8_t p, uint64_t imm);

		void emit_prologue();

		// Nothing else may emit code after emit_epilogue() except the miss
		// stubs (which the epilogue must not fall through into). guest_bytes /
		// guest_count are the total guest bytes / instructions of the block.
		void emit_epilogue(uint32_t guest_bytes, uint32_t guest_count);

		// Appends the TLB-miss stubs and patches every recorded jcc32 to its stub.
		void emit_miss_stubs();

		// Records a stub site for `instr`, reserving its share of stub space
		// and capturing the dirty-register snapshot at this point.
		void push_miss(uint32_t rel_pos, uint32_t instr);
		// Count of currently dirty guest registers (and their host slots).
		uint8_t snapshot_dirty(uint8_t* dv, uint8_t* ds);

		// Flush dirty regs and exit to the runner: exit_pc = entry_pc +
		// delta_va (or RAX), exit_count = count. Used by the control-transfer
		// translators as their block tail.
		void emit_block_exit(uint32_t delta_va, uint32_t count);
		void emit_block_exit_rax(uint32_t count);

		// Emits the shared go-to-dispatcher trampoline and the block return
		// stub, then patches every exit site's recorded branches onto them.
		void emit_link_stubs();

		// Branch: continue at entry_pc + off + size unless (rs1 cc rs2), which
		// takes entry_pc + off + imm. check_align exits an odd taken target as
		// a miss so the interpreter can trap on it.
		void emit_cond_exit(uint32_t rs1, uint32_t rs2, uint8_t cc, int64_t imm,
							uint32_t off, uint32_t size, uint32_t count_before, bool check_align);

		// C.BEQZ / C.BNEZ form: single register tested against zero.
		void emit_cond_exit_zero(uint32_t rs, uint8_t cc, int64_t imm,
								 uint32_t off, uint32_t count_before);

		// Jump ending the block: from_rs1 targets (GPR[src_rs1] + imm) & ~1
		// (JALR / C.JR / C.JALR), otherwise entry_pc + off + imm (JAL / C.J).
		// link_rd != 0 is written with entry_pc + off + size first.
		void emit_jump(uint32_t link_rd, uint32_t src_rs1, int64_t imm,
					   uint32_t off, uint32_t size, uint32_t count_before,
					   bool check_align, bool from_rs1);

		// Native guest loads/stores with an inline TLB lookup. width = bytes
		// (1/2/4/8); sign_extend only matters for narrow signed loads. The
		// instruction that faults is reported through blk->instr_index.
		void emit_load(uint32_t rd, uint32_t rs1, int64_t imm, uint8_t width, bool sign_extend);
		void emit_store(uint32_t rs1, int64_t imm, uint32_t rs2, uint8_t width);

		bool eof() const;

		void emit_r_to(uint8_t dstReg, uint8_t src1Reg, uint8_t src2Reg, ALUOp op, bool wVariant);
		void emit_i_to(uint8_t dstReg, uint8_t src1Reg, int64_t imm, ALUOp op, bool wVariant);
		// AUIPC resolves its pc from CTX_OFF_ENTRY at runtime (block-relative
		// offset comes from blk->instr_bytes), so no absolute VA is baked.
		void emit_u_to(uint8_t dstReg, int32_t imm, ALUOp op);
		void emit_m_r_to(uint8_t dstReg, uint8_t src1Reg, uint8_t src2Reg, MOp op, bool wVariant);
	};
}
