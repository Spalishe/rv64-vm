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

namespace rv64vm::jit
{
	struct JIT_Block
	{
		x86::CodeBuf code;
		uint64_t start_phys	 = 0;
		uint32_t count		 = 0;
		uint32_t bytes_guest = 0;
		uint64_t asid		 = 0;
		uint64_t smc_epoch	 = 0;
		bool valid			 = false;
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
		SLTU
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

		JIT_Emitter(JIT_Block* b);

		x86::CodeBuf& code() const;

		// Never evicts slots holding keep1/keep2 (may be live operands).
		uint8_t free_slot(uint32_t keep1 = 0xFFFFFFFFu, uint32_t keep2 = 0xFFFFFFFFu);

		uint8_t phys(uint8_t slot) const;

		uint8_t hreg_for_read(uint32_t v, uint32_t keep = 0xFFFFFFFFu);

		uint8_t hreg_for_write(uint32_t v, uint32_t keep1 = 0xFFFFFFFFu, uint32_t keep2 = 0xFFFFFFFFu);

		void flush_guest(uint32_t v);

		void flush_all();

		void load_imm64(uint8_t p, uint64_t imm);

		void emit_prologue();

		// Nothing else may emit code after emit_epilogue().
		void emit_epilogue(uint32_t guest_count);

		bool eof() const;

		void emit_r_to(uint8_t dstReg, uint8_t src1Reg, uint8_t src2Reg, ALUOp op, bool wVariant);
		void emit_i_to(uint8_t dstReg, uint8_t src1Reg, int64_t imm, ALUOp op, bool wVariant);
		void emit_m_r_to(uint8_t dstReg, uint8_t src1Reg, uint8_t src2Reg, MOp op, bool wVariant);
	};
}