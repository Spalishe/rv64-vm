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
 * x86-64 implementation of the block emitter / register allocator declared
 * in include/jit/rvjit_emit.hpp. All the host instruction encoding happens
 * here through the x86:: primitives from rvjit_x86_64.hpp.
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
		// Establish a real frame: the pushes below must never touch the
		// caller's red-zone/locals (they used to, corrupting whatever the
		// runner kept just below rsp). rbp isolates everything we push.
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
					// Save the shift count FIRST: when D aliases S2
					// (rd == rs2), moving S1 into D below would
					// overwrite the count before it reaches REG_TMP.
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
					// 0 <? S2
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
					// S1 <? 0
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

	void JIT_Emitter::emit_i_to(uint8_t dstReg, uint8_t src1Reg, int64_t imm, ALUOp op, bool wVariant)
	{
		const bool r1 = (src1Reg != 0);
		uint8_t S1	   = r1 ? hreg_for_read(src1Reg) : 0xFF;
		uint8_t D	   = hreg_for_write(dstReg, src1Reg, 0xFFFFFFFFu);
		switch(op)
		{
			case ALUOp::ADD: // ADDI
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