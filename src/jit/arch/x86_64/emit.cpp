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
 * x86-64 implementation of JIT_Emitter (declared in rvjit_emit.hpp).
 */
#include "../../../../include/jit/rvjit_emit.hpp"

namespace rv64vm::jit
{
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
			if(h != 0xFF && h != keep1 && h != keep2 && !vr[h].dirty)
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
			if(h != 0xFF && h != keep1 && h != keep2)
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
		// rbp frame keeps our pushes out of the caller's red zone.
		x86::push_r(code(), x86::REG_RBP);
		x86::mov_rr(code(), x86::REG_RBP, x86::REG_RSP);
		x86::push_r(code(), x86::REG_R13);
		x86::push_r(code(), x86::REG_R12);
		x86::mov_rr(code(), x86::REG_R12, x86::REG_RDI);
		x86::mov_mr(code(), x86::REG_R13, x86::REG_R12, x86::CTX_OFF_REGS);
	}

	void JIT_Emitter::emit_epilogue(uint32_t guest_count)
	{
		flush_all();
		x86::mov_mr(code(), x86::REG_RAX, x86::REG_CTX, x86::CTX_OFF_ENTRY);
		x86::add_imm(code(), x86::REG_RAX, (int32_t)(guest_count * 4));
		x86::mov_rm(code(), x86::REG_CTX, x86::CTX_OFF_EXIT, x86::REG_RAX);
		x86::pop_r(code(), x86::REG_R12);
		x86::pop_r(code(), x86::REG_R13);
		x86::pop_r(code(), x86::REG_RBP);
		x86::ret(code());
	}

	bool JIT_Emitter::eof() const
	{
		return code().pos + RVJIT_FUNC_MARGIN >= RVJIT_FUNC_SIZE;
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
		const bool r1	 = (src1Reg != 0), r2 = (src2Reg != 0);
		const bool mulF	 = (op == MOp::MUL || op == MOp::MULH || op == MOp::MULHU || op == MOp::MULHSU);
		const bool rem	 = (op == MOp::REM || op == MOp::REMU);
		const bool signed_ = (op == MOp::MULH || op == MOp::MULHSU || op == MOp::DIV || op == MOp::REM);
		x86::CodeBuf& cb = code();

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
				if(wVariant) { x86::imul_rr32(cb, D, S2); x86::movsxd(cb, D, D); }
				else x86::imul_rr(cb, D, S2);
			}
			else if(D == S2)
			{
				if(wVariant) { x86::imul_rr32(cb, D, S1); x86::movsxd(cb, D, D); }
				else x86::imul_rr(cb, D, S1);
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
			uint32_t fix_done1	 = x86::jcc8(cb, 0xEB);
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
		uint8_t S1	   = r1 ? hreg_for_read(src1Reg) : 0xFF;
		uint8_t D	   = hreg_for_write(dstReg, src1Reg, 0xFFFFFFFFu);
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
}