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

/*
 * RV64I ALU register/immediate translators.
 *
 * Semantics mirror src/sets/rv64i.cpp exactly:
 *  - 64-bit ops: full-width GPR arithmetic, shifts use rs2 & 0x3f.
 *  - W variants: 32-bit wrap and sign-extension to 64 bits.
 *  - x0 writes are dropped (spec).
 *
 * Each function returns true when the block may keep compiling past this
 * instruction and false when the block must stop right after it (buffer
 * nearly exhausted).
 */
#include "../../../../include/decode.hpp"
#include "../../../../include/jit/rvjit.hpp"

#ifdef USE_JIT

namespace rv64vm::jit
{
	using namespace rv64vm::runner;

	static inline bool alu_r(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, ALUOp op, bool w)
	{
		if(d.rd == 0)
			return !em.eof(); // x0 writes are dropped
		em.emit_r_to(d.rd, d.rs1, d.rs2, op, w);
		return !em.eof();
	}
	static inline bool alu_i(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, ALUOp op, bool w)
	{
		if(d.rd == 0)
			return !em.eof(); // x0 writes are dropped
		em.emit_i_to(d.rd, d.rs1, (int64_t)d.imm, op, w);
		return !em.eof();
	}

	// R-Type
	bool jit_ADD(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::ADD, false);
	}
	bool jit_ADDW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::ADD, true);
	}
	bool jit_SUB(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SUB, false);
	}
	bool jit_SUBW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SUB, true);
	}
	bool jit_XOR(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::XOR, false);
	}
	bool jit_OR(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::OR, false);
	}
	bool jit_AND(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::AND, false);
	}
	bool jit_SLL(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SLL, false);
	}
	bool jit_SLLW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SLL, true);
	}
	bool jit_SRL(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SRL, false);
	}
	bool jit_SRLW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SRL, true);
	}
	bool jit_SRA(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SRA, false);
	}
	bool jit_SRAW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SRA, true);
	}
	bool jit_SLT(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SLT, false);
	}
	bool jit_SLTU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_r(h, d, b, e, ALUOp::SLTU, false);
	}

	// I-Type (ALU)
	bool jit_ADDI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::ADD, false);
	}
	bool jit_ADDIW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::ADD, true);
	}
	bool jit_XORI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::XOR, false);
	}
	bool jit_ORI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::OR, false);
	}
	bool jit_ANDI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::AND, false);
	}
	bool jit_SLLI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SLL, false);
	}
	bool jit_SLLIW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SLL, true);
	}
	bool jit_SRLI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SRL, false);
	}
	bool jit_SRLIW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SRL, true);
	}
	bool jit_SRAI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SRA, false);
	}
	bool jit_SRAIW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SRA, true);
	}
	bool jit_SLTI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SLT, false);
	}
	bool jit_SLTIU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return alu_i(h, d, b, e, ALUOp::SLTU, false);
	}
}

#endif
