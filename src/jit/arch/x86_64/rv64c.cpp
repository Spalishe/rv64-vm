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

#include "../../../../include/decode.hpp"
#include "../../../../include/hart.hpp"
#include "../../../../include/jit/rvjit.hpp"

#ifdef USE_JIT

namespace rv64vm::jit
{
	using namespace rv64vm::runner;

	// Compressed ALU translators. These mirror the interpreter bodies in
	// src/sets/rv64c.cpp field-by-field (d_c_rd/d_c_rs1/d_c_rs2 read the low
	// 16 bits of d.inst). Only ALU / register moves are handled here; the
	// control-transfer and FP compressed instructions keep jit_func == nullptr
	// so the block ends and the interpreter takes them.
	static inline bool c_keep(JIT_Emitter& em)
	{
		return !em.eof();
	}

	bool jit_C_NOP(Hart&, InstructionData&, JIT_Block&, JIT_Emitter& em)
	{
		return c_keep(em);
	}

	// C.ADDI4SPN: x[8+rd'] = sp + nzuimm (rd' in inst[4:2]).
	bool jit_C_ADDI4SPN(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		em.emit_i_to(8 + (uint8_t)d_c_rd(d.inst), 2, (int64_t)d.imm, ALUOp::ADD, false);
		return c_keep(em);
	}

	// C.ADDI: rd += sext6(imm); C.NOP / hints write nothing.
	bool jit_C_ADDI(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		if(d.rd == 0)
			return c_keep(em);
		em.emit_i_to(d.rd, d.rd, (int64_t)d.imm, ALUOp::ADD, false);
		return c_keep(em);
	}

	// C.ADDIW: rd = (int32)(rd + sext6(imm)).
	bool jit_C_ADDIW(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		if(d.rd == 0)
			return c_keep(em);
		em.emit_i_to(d.rd, d.rd, (int64_t)d.imm, ALUOp::ADD, true);
		return c_keep(em);
	}

	// C.LI: rd = sext6(imm).
	bool jit_C_LI(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		if(d.rd == 0)
			return c_keep(em);
		em.emit_i_to(d.rd, 0, (int64_t)d.imm, ALUOp::ADD, false);
		return c_keep(em);
	}

	// C.ADDI16SP (rd == 2): sp += nzimm[9:4]; otherwise C.LUI: rd = sext6(imm)<<12.
	bool jit_C_LUI_ADDI16SP(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		if(d.rd == 2)
		{
			em.emit_i_to(2, 2, (int64_t)sext(d_c_nzimm_9(d.inst), 10), ALUOp::ADD, false);
		}
		else if(d.rd != 0)
		{
			em.emit_i_to(d.rd, 0, (int64_t)(d.imm << 12), ALUOp::ADD, false);
		}
		return c_keep(em);
	}

	// C.SLLI: rd <<= shamt; hints (rd == 0) write nothing.
	bool jit_C_SLLI(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		if(d.rd == 0)
			return c_keep(em);
		em.emit_i_to(d.rd, d.rd, (int64_t)d.imm, ALUOp::SLL, false);
		return c_keep(em);
	}

	// CB register-form helpers: rd' lives in inst[9:7], shamt/imm[4:0] in inst[6:2].
	static inline bool c_shift(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, ALUOp op)
	{
		const uint8_t rd = 8 + (uint8_t)d_c_rs1(d.inst);
		em.emit_i_to(rd, rd, (int64_t)d.imm, op, false);
		return c_keep(em);
	}

	bool jit_C_SRLI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_shift(h, d, b, em, ALUOp::SRL);
	}
	bool jit_C_SRAI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_shift(h, d, b, em, ALUOp::SRA);
	}

	// C.ANDI: rd &= sext6(imm).
	bool jit_C_ANDI(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		const uint8_t rd = 8 + (uint8_t)d_c_rs1(d.inst);
		em.emit_i_to(rd, rd, (int64_t)sext(d.imm, 6), ALUOp::AND, false);
		return c_keep(em);
	}

	// CA register form: rd' in inst[9:7], rs2' in inst[4:2].
	static inline bool c_alu(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, ALUOp op, bool w)
	{
		const uint8_t rd = 8 + (uint8_t)d_c_rs1(d.inst);
		const uint8_t rs = 8 + (uint8_t)d_c_rd(d.inst);
		em.emit_r_to(rd, rd, rs, op, w);
		return c_keep(em);
	}

	bool jit_C_SUB(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_alu(h, d, b, em, ALUOp::SUB, false);
	}
	bool jit_C_XOR(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_alu(h, d, b, em, ALUOp::XOR, false);
	}
	bool jit_C_OR(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_alu(h, d, b, em, ALUOp::OR, false);
	}
	bool jit_C_AND(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_alu(h, d, b, em, ALUOp::AND, false);
	}
	bool jit_C_SUBW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_alu(h, d, b, em, ALUOp::SUB, true);
	}
	bool jit_C_ADDW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_alu(h, d, b, em, ALUOp::ADD, true);
	}

	// C.MV: rd = rs2 (rs2 in inst[6:2]); hints (rd == 0) write nothing.
	bool jit_C_MV(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		if(d.rd == 0)
			return c_keep(em);
		em.emit_r_to(d.rd, 0, (uint8_t)d_c_rs2(d.inst), ALUOp::ADD, false);
		return c_keep(em);
	}

	// C.ADD: rd += rs2; hints (rd == 0) write nothing.
	bool jit_C_ADD(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em)
	{
		if(d.rd == 0)
			return c_keep(em);
		em.emit_r_to(d.rd, d.rd, (uint8_t)d_c_rs2(d.inst), ALUOp::ADD, false);
		return c_keep(em);
	}

	// CL/CS index-form load/store: rd' in inst[4:2], rs1' in inst[9:7]; the
	// rs2 of C.SW/C.SD reuses the rd' field. Mirrors the interpreter bodies
	// (which index GPR[8 + d_c_rs1] / GPR[8 + d_c_rd]) and the RV64I JIT
	// load/store wrappers.
	static inline bool c_ld(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, uint8_t width, bool sign)
	{
		em.emit_load(8 + (uint8_t)d_c_rd(d.inst), 8 + (uint8_t)d_c_rs1(d.inst), (int64_t)d.imm, width, sign);
		return c_keep(em);
	}

	static inline bool c_st(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, uint8_t width)
	{
		em.emit_store(8 + (uint8_t)d_c_rs1(d.inst), (int64_t)d.imm, 8 + (uint8_t)d_c_rd(d.inst), width);
		return c_keep(em);
	}

	bool jit_C_LW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_ld(h, d, b, em, 4, true);
	}
	bool jit_C_LD(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_ld(h, d, b, em, 8, true);
	}
	bool jit_C_SW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_st(h, d, b, em, 4);
	}
	bool jit_C_SD(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_st(h, d, b, em, 8);
	}

	// CSP stack-relative load/store: rs1 = sp, the destination of C.LWSP /
	// C.LDSP sits in inst[11:7] (InstructionData.rd), the source of C.SWSP /
	// C.SDSP in inst[6:2] (d_c_rs2).
	static inline bool c_ld_sp(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, uint8_t width, bool sign)
	{
		em.emit_load((uint8_t)d.rd, 2, (int64_t)d.imm, width, sign);
		return c_keep(em);
	}

	static inline bool c_st_sp(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, uint8_t width)
	{
		em.emit_store(2, (int64_t)d.imm, (uint8_t)d_c_rs2(d.inst), width);
		return c_keep(em);
	}

	bool jit_C_LWSP(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_ld_sp(h, d, b, em, 4, true);
	}
	bool jit_C_LDSP(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_ld_sp(h, d, b, em, 8, true);
	}
	bool jit_C_SWSP(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_st_sp(h, d, b, em, 4);
	}
	bool jit_C_SDSP(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em)
	{
		return c_st_sp(h, d, b, em, 8);
	}
}

#endif