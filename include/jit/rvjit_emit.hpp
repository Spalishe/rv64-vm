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
 * Guest-code translator support: block buffer + virtual/host register
 * allocator used by the RV64I instruction translators.
 *
 * A "virtual register" is a guest register number 0..31; the allocator maps
 * a small window of them onto host registers. All 32 guest registers live
 * in memory at REGS = &hart.GPR[0]; a guest register only occupies a host
 * register while the surrounding instructions use it. Dirty values are
 * flushed back to memory at the end of the block (flush_all in the
 * epilogue) or when the host register is reused.
 *
 * This header declares the interface only. The JIT_Emitter implementation
 * (which encodes x86-64 host instructions) lives in the architecture
 * specific translation unit src/jit/arch/x86_64/emit.cpp.
 */
#include "rvjit_x86_64.hpp"
#include <cstdint>

namespace rv64vm::jit
{
	// ------------------------------------------------------------------
	// One compiled block of guest straight-line code.
	// ------------------------------------------------------------------
	struct JIT_Block
	{
		x86::CodeBuf code;		  // host machine code being produced
		uint64_t start_phys	 = 0; // physical guest pc of the first instruction
		uint32_t count		 = 0; // guest instructions compiled
		uint32_t bytes_guest = 0; // guest byte span (count * 4)
		uint64_t asid		 = 0; // satp.ASID at compile time
		uint64_t smc_epoch	 = 0; // g_smc_epoch at compile time
		bool valid			 = false;
	};

	// ------------------------------------------------------------------
	// Track which guest register lives in which host register.
	// ------------------------------------------------------------------
	struct VReg
	{
		int8_t slot = -1; // host pool slot index, -1 = not cached
		bool dirty	= false;
	};

	// ------------------------------------------------------------------
	// High level ALU operations the translators can request.
	// ------------------------------------------------------------------
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
		SLTU
	};

	// ------------------------------------------------------------------
	// Block emitter / register allocator.
	//
	// register form: rd = rs1 <op> rs2
	// Operands are always loaded into host registers BEFORE the
	// destination slot is allocated, and the destination allocation is
	// forbidden from evicting those live operand slots.  Without this
	// ordering, free_slot() can recycle a source (or the destination)
	// mid-instruction and the ALU op silently computes on a stale value.
	// For SLL/SRL/SRA (R-type), rs2 is a variable shift count in GPR[rs2].
	// For SLLI/SRLI/SRAI (I-type), the shift amount is an immediate
	// handled by emit_i_to.
	// ------------------------------------------------------------------
	struct JIT_Emitter
	{
		JIT_Block* blk;
		VReg vr[32];
		uint8_t hr_vreg[x86::RVJIT_HOST_REGS]; // guest reg held in each slot, 0xFF = free

		JIT_Emitter(JIT_Block* b);

		x86::CodeBuf& code() const;

		// ---- allocation ------------------------------------------------
		// Returns the index of a free host slot, never evicting slots that
		// currently hold the guest registers keep1/keep2 (they may be live
		// operands of the instruction being compiled).
		uint8_t free_slot(uint32_t keep1 = 0xFFFFFFFFu, uint32_t keep2 = 0xFFFFFFFFu);

		uint8_t phys(uint8_t slot) const;

		uint8_t hreg_for_read(uint32_t v, uint32_t keep = 0xFFFFFFFFu);

		uint8_t hreg_for_write(uint32_t v, uint32_t keep1 = 0xFFFFFFFFu, uint32_t keep2 = 0xFFFFFFFFu);

		void flush_guest(uint32_t v);

		void flush_all();

		// ---- registers used by the generated code ----------------------
		void load_imm64(uint8_t p, uint64_t imm);

		// ---- block start / end -----------------------------------------
		void emit_prologue();

		// Nothing else may emit code after emit_epilogue().
		void emit_epilogue(uint32_t guest_count);

		// ---- minimum buffer sanity -------------------------------------
		bool eof() const;

		void emit_r_to(uint8_t dstReg, uint8_t src1Reg, uint8_t src2Reg, ALUOp op, bool wVariant);
		void emit_i_to(uint8_t dstReg, uint8_t src1Reg, int64_t imm, ALUOp op, bool wVariant);
	};
}