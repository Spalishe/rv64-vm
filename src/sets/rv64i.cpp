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

#include "../../include/decode.hpp"
#include "../../include/hart.hpp"
#include <cstddef>

// R-Type

ExecReturn exec_ADDW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] + (uint32_t)hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SUBW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] - (uint32_t)hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLLW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] << ((uint32_t)hart.GPR[inst.rs2] & 0x1F));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRLW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] >> ((uint32_t)hart.GPR[inst.rs2] & 0x1F));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRAW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)(((int32_t)hart.GPR[inst.rs1]) >> ((uint32_t)hart.GPR[inst.rs2] & 0x1F));
	return { true, false, 4, 0, 0 };
}

// I-Type
ExecReturn exec_ADDIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] + inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLLIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] << inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRLIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] >> inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRAIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(((int32_t)hart.GPR[inst.rs1]) >> inst.imm);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_LD(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint64_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Long, &val);

	if(success.is_success)
	{
		hart.GPR[inst.rd] = val;
	}

	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LWU(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint32_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Int, &val);

	if(success.is_success)
	{
		hart.GPR[inst.rd] = val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}

// S-Type
ExecReturn exec_SD(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Long, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}

/// RV32I

// R-Type

ExecReturn exec_ADD(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] + hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SUB(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] - hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_XOR(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] ^ hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_OR(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] | hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_AND(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLL(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] << (hart.GPR[inst.rs2] & 0x3f);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRL(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] >> (hart.GPR[inst.rs2] & 0x3f);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRA(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1]) >> (hart.GPR[inst.rs2] & 0x3f);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLT(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1] < (int64_t)hart.GPR[inst.rs2]) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLTU(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (hart.GPR[inst.rs1] < hart.GPR[inst.rs2]) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}

// I-Type
ExecReturn exec_ADDI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_XORI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] ^ inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ORI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] | inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ANDI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLLI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] << inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRLI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] >> inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRAI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1]) >> inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLTI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1] < (int64_t)inst.imm) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLTIU(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (hart.GPR[inst.rs1] < inst.imm) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_LB(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	int8_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Byte, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LH(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	int16_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Short, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LW(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	int32_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Int, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LBU(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint8_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Byte, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)(uint8_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LHU(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint16_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Short, &val);

	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}

// S-Type
ExecReturn exec_SB(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Byte, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}
ExecReturn exec_SH(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Short, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}
ExecReturn exec_SW(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Int, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}

// B-Type
ExecReturn exec_BEQ(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] == (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BNE(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] != (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BLT(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] < (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
		{
			hart.pc = hart.pc + (int64_t)inst.imm;
		}
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BGE(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] >= (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BLTU(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs1] < hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;

		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BGEU(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs1] >= hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;

		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}

// JUMP
ExecReturn exec_JAL(Hart& hart, InstructionData& inst)
{
	uint64_t tmp = hart.pc;
	if((hart.pc + (int64_t)inst.imm) % 2 != 0)
	{
		return { false, true, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
	}
	else
	{
		hart.pc			  = hart.pc + (int64_t)inst.imm;
		hart.GPR[inst.rd] = tmp + 4;
	}
	return { true, true, 0, 0, 0 };
}
ExecReturn exec_JALR(Hart& hart, InstructionData& inst)
{
	uint64_t tmp	= hart.pc;
	uint64_t target = (hart.GPR[inst.rs1] + (int64_t)inst.imm) & ~1;
	if(target % 2 != 0)
	{
		return { false, true, 0, EXC_INST_ADDR_MISALIGNED, target };
	}
	else
	{
		hart.pc			  = target;
		hart.GPR[inst.rd] = tmp + 4;
	}
	return { true, true, 0, 0, 0 };
}

// what

ExecReturn exec_LUI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)((uint64_t)inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_AUIPC(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.pc + (int64_t)((uint64_t)inst.imm);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_ECALL(Hart& hart, InstructionData& inst)
{

	switch(hart.mode)
	{
		case PrivilegeMode::Machine:
			return { false, true, 0, EXC_ENV_CALL_FROM_M, 0 };
		case PrivilegeMode::Supervisor:
			return { false, true, 0, EXC_ENV_CALL_FROM_S, 0 };
		case PrivilegeMode::User:
			return { false, true, 0, EXC_ENV_CALL_FROM_U, 0 };
		default:
			// How did we get here?
			return { true, false, 4, 0, 0 };
	}
}
ExecReturn exec_EBREAK(Hart& hart, InstructionData& inst)
{
	return { false, true, 0, EXC_BREAKPOINT, hart.pc };
}

ExecReturn exec_FENCE(Hart& hart, InstructionData& inst)
{
	// nop
	// If you planning adding some memory write/read buffer, you have to implement this instruction then
	// FENCE guaranties that all cores will see all changes that have done by 1 specific core before FENCE instruction

	return { true, false, 4, 0, 0 };
}

#ifdef USE_JIT
#include "../../include/rvjit/rvjit_x86_64.hpp"
bool execjit_ADD(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			add_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			add_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			add_rr(blk, rd.host_reg, rs1.host_reg);
		}
	}, blk.pc + blk.size);
	return false;
}
bool execjit_ADDW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			add_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			add_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			add_rr32(blk, rd.host_reg, rs1.host_reg);
		}

		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SUB(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			if(rd.vreg == rs2.vreg)
			{
				neg(blk, rd.host_reg);
			}
			else
			{
				mov(blk, rd.host_reg, rs2.host_reg);
				neg(blk, rd.host_reg);
			}
			return;
		}
		if(rs2.vreg == 0)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			sub_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			mov(blk, REG_RCX, rs2.host_reg);
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr(blk, rd.host_reg, REG_RCX);
		}
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SUBW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			if(rd.vreg == rs2.vreg)
			{
				neg32(blk, rd.host_reg);
			}
			else
			{
				mov(blk, rd.host_reg, rs2.host_reg);
				neg32(blk, rd.host_reg);
			}
			return;
		}
		if(rs2.vreg == 0)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			sub_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			mov(blk, REG_RCX, rs2.host_reg);
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr32(blk, rd.host_reg, REG_RCX);
		}
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_XOR(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			xor_rr(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		xor_rr(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_OR(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			or_rr(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		or_rr(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_AND(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rs1.is_zero || rs2.is_zero)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			and_rr(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		and_rr(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLL(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shl_rc(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_rc(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLLW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shl_rc32(blk, rd.host_reg, REG_RCX);
			movsxd(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_rc32(blk, rd.host_reg, rs2.host_reg);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRL(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shr_rc(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_rc(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRLW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shr_rc32(blk, rd.host_reg, REG_RCX);
			movsxd(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_rc32(blk, rd.host_reg, rs2.host_reg);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRA(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			sar_rc(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_rc(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRAW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			sar_rc32(blk, rd.host_reg, REG_RCX);
			movsxd(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_rc32(blk, rd.host_reg, rs2.host_reg);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLT(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false,
							 [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0 || rs2.vreg == 0)
			xor_rr(blk, REG_RCX, REG_RCX);
		if(rs1.vreg == 0)
			cmp(blk, REG_RCX, rs2.host_reg);
		else if(rs2.vreg == 0)
			cmp(blk, rs1.host_reg, REG_RCX);
		else
			cmp(blk, rs1.host_reg, rs2.host_reg);
		setl(blk, rd.host_reg);
		movzx(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLTU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0 || rs2.vreg == 0)
			xor_rr(blk, REG_RCX, REG_RCX);
		if(rs1.vreg == 0)
			cmp(blk, REG_RCX, rs2.host_reg);
		else if(rs2.vreg == 0)
			cmp(blk, rs1.host_reg, REG_RCX);
		else
			cmp(blk, rs1.host_reg, rs2.host_reg);

		setb(blk, rd.host_reg);
		movzx(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}

bool execjit_ADDI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}
		add_rimm32(blk, rd.host_reg, (int32_t)imm);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_ADDIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			mov_imm64(blk, rd.host_reg, imm);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}
		add_r32imm32(blk, rd.host_reg, (int32_t)imm);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_XORI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}
		xor_rimm32(blk, rd.host_reg, imm);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_ORI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		or_rimm32(blk, rd.host_reg, imm);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_ANDI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.is_zero || imm == 0)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		and_rimm32(blk, rd.host_reg, imm);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLLI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_rimm8(blk, rd.host_reg, imm & 0x3F);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLLIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			xor_rr32(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_r32imm8(blk, rd.host_reg, imm & 0x1F);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRLI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_rimm8(blk, rd.host_reg, imm & 0x3F);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRLIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			xor_rr32(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_r32imm8(blk, rd.host_reg, imm & 0x1F);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRAI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_rimm8(blk, rd.host_reg, imm & 0x3F);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SRAIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.vreg == 0)
		{
			xor_rr32(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_r32imm8(blk, rd.host_reg, imm & 0x1F);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLTI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false,
							 [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		mov_imm64(blk, REG_RCX, imm);

		if(rs1.vreg == 0)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			cmp(blk, rd.host_reg, REG_RCX);
		}
		else
			cmp(blk, rs1.host_reg, REG_RCX);
		setl(blk, rd.host_reg);
		movzx(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}
bool execjit_SLTIU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		mov_imm64(blk, REG_RCX, imm);

		if(rs1.vreg == 0)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			cmp(blk, rd.host_reg, REG_RCX);
		}
		else
			cmp(blk, rs1.host_reg, REG_RCX);
		setb(blk, rd.host_reg);
		movzx(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
	return false;
}

uint64_t jit_slow_lb(Hart* h, uint64_t addr)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Byte + 1))
		{
			// found a device
			int8_t out = dev->read(addr, MemorySize::Byte);
			return (uint64_t)out;
		}
	}
	return 0;
}
uint64_t jit_slow_lbu(Hart* h, uint64_t addr)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Byte + 1))
		{
			// found a device
			uint8_t out = dev->read(addr, MemorySize::Byte);
			return (uint64_t)out;
		}
	}
	return 0;
}
uint64_t jit_slow_lh(Hart* h, uint64_t addr)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Short + 1))
		{
			// found a device
			int16_t out = dev->read(addr, MemorySize::Short);
			return (uint64_t)out;
		}
	}
	return 0;
}
uint64_t jit_slow_lhu(Hart* h, uint64_t addr)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Short + 1))
		{
			// found a device
			uint16_t out = dev->read(addr, MemorySize::Short);
			return (uint64_t)out;
		}
	}
	return 0;
}
uint64_t jit_slow_lw(Hart* h, uint64_t addr)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Int + 1))
		{
			// found a device
			int32_t out = dev->read(addr, MemorySize::Int);
			return (uint64_t)out;
		}
	}
	return 0;
}
uint64_t jit_slow_lwu(Hart* h, uint64_t addr)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Int + 1))
		{
			// found a device
			uint32_t out = dev->read(addr, MemorySize::Int);
			return (uint64_t)out;
		}
	}
	return 0;
}
uint64_t jit_slow_ld(Hart* h, uint64_t addr)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Long + 1))
		{
			// found a device
			uint64_t out = dev->read(addr, MemorySize::Long);
			return out;
		}
	}
	return 0;
}

using SlowMemFunc = uint64_t (*)(Hart*, uint64_t);
struct jit_memory_op
{
	void* fast_mov;
	void* slow_find;
};

bool jit_load(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter, void* func, void* func_slow)
{
	jit_memory_op stru = jit_memory_op{ func, func_slow };
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		mov(blk, REG_RCX, rs1.host_reg);
		add_rimm32(blk, REG_RCX, (int32_t)imm);
		sub_rimm32(blk, REG_RCX, 0x40000000); //
		sub_rimm32(blk, REG_RCX, 0x40000000); // This does sum of 0x80000000, which is beyond the int32_t limit
		cmp_rm(blk, REG_RCX, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, memsize));

		blk.jmp_labels.push_back({ "fast_path", blk.byte_pos, false, 1 });
		jbe8(blk, 0);

		auto function_data = *reinterpret_cast<jit_memory_op*>(tmp);
		{
			// Slow path, make interpreter work instead
			/*mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, exit_pc));
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);*/
			// old method, returning to interpreter
			/*push(blk, REG_RDI);
			push(blk, REG_RSI);
			push(blk, REG_RAX);*/
			push(blk, REG_RAX);
			push(blk, REG_RDX);
			push(blk, REG_RSI);
			push(blk, REG_RDI);
			push(blk, REG_R8);
			push(blk, REG_R9);
			push(blk, REG_R10);
			push(blk, REG_R11);

			mov(blk, REG_RSI, REG_RCX);
			mov_rm(blk, REG_RDI, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, hart));
			mov_imm64(blk, REG_RAX, (uint64_t)function_data.slow_find);
			call(blk, REG_RAX);
			mov(blk, REG_RCX, REG_RAX);
			/*pop(blk, REG_RAX);
			pop(blk, REG_RSI);
			pop(blk, REG_RDI);   */
			pop(blk, REG_R11);
			pop(blk, REG_R10);
			pop(blk, REG_R9);
			pop(blk, REG_R8);
			pop(blk, REG_RDI);
			pop(blk, REG_RSI);
			pop(blk, REG_RDX);
			pop(blk, REG_RAX);
			mov(blk, rd.host_reg, REG_RCX);

			blk.jmp_labels.push_back({ "end", blk.byte_pos, false, 1 });
			jmp8(blk, 0);
		}

		em.realize_label(blk, "fast_path");

		auto function_ptr = reinterpret_cast<MovSignature>(function_data.fast_mov);
		function_ptr(blk, rd.host_reg, REG_R14, REG_RCX, 0, 0);
		em.realize_label(blk, "end");
	}, blk.pc + blk.size, reinterpret_cast<void*>(&stru));
	return false;
}
bool execjit_LB(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movsx_r64m8), reinterpret_cast<void*>(&jit_slow_lb));
}
bool execjit_LBU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movzx_r32m8), reinterpret_cast<void*>(&jit_slow_lbu));
}
bool execjit_LH(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movsx_r64m16), reinterpret_cast<void*>(&jit_slow_lh));
}
bool execjit_LHU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movzx_r32m16), reinterpret_cast<void*>(&jit_slow_lhu));
}
bool execjit_LW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movsxd_r64m32), reinterpret_cast<void*>(&jit_slow_lw));
}
bool execjit_LWU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_r32m), reinterpret_cast<void*>(&jit_slow_lwu));
}
bool execjit_LD(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_rm), reinterpret_cast<void*>(&jit_slow_ld));
}
void jit_slow_sb(Hart* h, uint64_t addr, uint64_t val)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Byte + 1))
		{
			// found a device
			dev->write(addr, MemorySize::Byte, val);
			return;
		}
	}
}

void jit_slow_sh(Hart* h, uint64_t addr, uint64_t val)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Short + 1))
		{
			// found a device
			dev->write(addr, MemorySize::Short, val);
			return;
		}
	}
}

void jit_slow_sw(Hart* h, uint64_t addr, uint64_t val)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Int + 1))
		{
			// found a device
			dev->write(addr, MemorySize::Int, val);
			return;
		}
	}
}

void jit_slow_sd(Hart* h, uint64_t addr, uint64_t val)
{
	// We only know about phys addr
	addr += 0x80000000;
	for(const auto& dev : h->mmio->devs)
	{
		if(addr >= dev->start && addr < (dev->start + dev->size - (int)MemorySize::Long + 1))
		{
			// found a device
			dev->write(addr, MemorySize::Long, val);
			return;
		}
	}
}
bool jit_store(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter, void* func, void* func_slow)
{
	jit_memory_op stru = jit_memory_op{ func, func_slow };
	emitter.inst_emit_s_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rs1, VReg& rs2, uint64_t imm, uint64_t pc, void* tmp)
	{
		mov(blk, REG_RCX, rs1.host_reg);
		add_rimm32(blk, REG_RCX, (int32_t)imm);
		sub_rimm32(blk, REG_RCX, 0x40000000); //
		sub_rimm32(blk, REG_RCX, 0x40000000); // This does sum of 0x80000000, which is beyond the int32_t limit
		cmp_rm(blk, REG_RCX, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, memsize));

		blk.jmp_labels.push_back({ "fast_path", blk.byte_pos, false, 1 });
		jbe8(blk, 0);

		auto function_data = *reinterpret_cast<jit_memory_op*>(tmp);
		{
			// Slow path, make interpreter work instead
			/*mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, 32);
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);*/

			/*push(blk, REG_RDI);
			push(blk, REG_RSI);
			push(blk, REG_RAX);
			push(blk, REG_RDX);*/

			push(blk, REG_RAX);
			push(blk, REG_RDX);
			push(blk, REG_RSI);
			push(blk, REG_RDI);
			push(blk, REG_R8);
			push(blk, REG_R9);
			push(blk, REG_R10);
			push(blk, REG_R11);

			if(rs2.vreg == 0)
				xor_rr(blk, REG_RDX, REG_RDX);
			else
				mov(blk, REG_RDX, rs2.host_reg);

			mov(blk, REG_RSI, REG_RCX);
			mov_rm(blk, REG_RDI, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, hart));
			mov_imm64(blk, REG_RAX, (uint64_t)function_data.slow_find);
			call(blk, REG_RAX);

			pop(blk, REG_R11);
			pop(blk, REG_R10);
			pop(blk, REG_R9);
			pop(blk, REG_R8);
			pop(blk, REG_RDI);
			pop(blk, REG_RSI);
			pop(blk, REG_RDX);
			pop(blk, REG_RAX);

			/*pop(blk, REG_RDX);
			pop(blk, REG_RAX);
			pop(blk, REG_RSI);
			pop(blk, REG_RDI);*/
			blk.jmp_labels.push_back({ "end", blk.byte_pos, false, 1 });
			jmp8(blk, 0);
		}

		em.realize_label(blk, "fast_path");

		auto function_ptr = reinterpret_cast<MovSignature>(function_data.fast_mov);
		if(rs2.vreg == 0)
		{
			push(blk, REG_RAX);
			xor_rr(blk, REG_RAX, REG_RCX);
			function_ptr(blk, REG_RAX, REG_R14, REG_RCX, 0, 0);
			pop(blk, REG_RAX);
		}
		else
		{
			function_ptr(blk, rs2.host_reg, REG_R14, REG_RCX, 0, 0);
		}

		em.realize_label(blk, "end");
	}, blk.pc + blk.size, reinterpret_cast<void*>(&stru));
	return false;
}
bool execjit_SB(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_store(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_m8r8), reinterpret_cast<void*>(&jit_slow_sb));
}
bool execjit_SH(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_store(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_m16r16), reinterpret_cast<void*>(&jit_slow_sh));
}
bool execjit_SW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_store(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_m32r32), reinterpret_cast<void*>(&jit_slow_sw));
}
bool execjit_SD(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_store(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_mr), reinterpret_cast<void*>(&jit_slow_sd));
}

/*
 *	The reason why we need diff functions for BLTU and BGEU:
 *	There's no Jump function for Unsigned ops for test.
 *	So if both regs are zero:
 *		BLTU: never (x0 < x0)
 *		BGEU: always (x0 >= x0)
 *	if rs2 are zero:
 *		BLTU: never (unsigned rs1 never can be less than zero)
 *		BGEU: always (unsigned rs2 will be always greater or equal than zero)
 *	if rs1 are zero:
 *		BLTU: test rs2,rs2; jnz
 *		BGEU: test rs2,rs2; jz
 */

enum class branch_type
{
	BEQ,
	BNE,
	BLT,
	BGE,
	BLTU,
	BGEU
};

bool jit_branch(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter, branch_type type)
{
	emitter.inst_emit_b_type(
		hart,
		inst,
		blk,
		[](JIT_Emitter& em, JIT_Block& blk, VReg& rs1, VReg& rs2, uint64_t imm, uint64_t pc, void* tmp)
	{
		const branch_type type = *reinterpret_cast<branch_type*>(tmp);

		auto emit_taken_jcc = [&](Jmp8Signature jcc_fn)
		{
			blk.jmp_labels.push_back({ "taken", blk.byte_pos, false, 1 });
			jcc_fn(blk, 0);
		};

		auto emit_taken_jmp = [&]()
		{
			blk.jmp_labels.push_back({ "taken", blk.byte_pos, false, 1 });
			jmp8(blk, 0);
		};

		// Always keep loop-limit check first.
		sub_mimm32(blk, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, loop_count), blk.count);
		blk.jmp_labels.push_back({ "slow_path", blk.byte_pos, false, 1 });
		js8(blk, 0);

		const bool rs1_zero = (rs1.vreg == 0);
		const bool rs2_zero = (rs2.vreg == 0);

		// Compile-time fold when both operands are x0.
		if(rs1_zero && rs2_zero)
		{
			switch(type)
			{
				case branch_type::BEQ:
				case branch_type::BGE:
				case branch_type::BGEU:
					emit_taken_jmp();
					break;

				case branch_type::BNE:
				case branch_type::BLT:
				case branch_type::BLTU:
					// never taken, fallthrough
					return;
			}
		}
		else if(rs1_zero)
		{
			// 0 ? rs2
			test(blk, rs2.host_reg, rs2.host_reg);

			switch(type)
			{
				case branch_type::BEQ:
					emit_taken_jcc(&je8);
					break;
				case branch_type::BNE:
					emit_taken_jcc(&jne8);
					break;
				case branch_type::BLT:
					emit_taken_jcc(&jg8);
					break; // 0 < rs2  => rs2 > 0
				case branch_type::BGE:
					emit_taken_jcc(&jle8);
					break; // 0 >= rs2 => rs2 <= 0
				case branch_type::BLTU:
					emit_taken_jcc(&jne8);
					break; // 0 <u rs2 => rs2 != 0
				case branch_type::BGEU:
					emit_taken_jcc(&je8);
					break; // 0 >=u rs2 => rs2 == 0
			}
		}
		else if(rs2_zero)
		{
			// rs1 ? 0
			test(blk, rs1.host_reg, rs1.host_reg);

			switch(type)
			{
				case branch_type::BEQ:
					emit_taken_jcc(&je8);
					break;
				case branch_type::BNE:
					emit_taken_jcc(&jne8);
					break;
				case branch_type::BLT:
					emit_taken_jcc(&js8);
					break; // rs1 < 0
				case branch_type::BGE:
					emit_taken_jcc(&jns8);
					break; // rs1 >= 0
				case branch_type::BLTU:
					// rs1 <u 0 is never true
					return;
				case branch_type::BGEU:
					// rs1 >=u 0 is always true
					emit_taken_jmp();
					break;
			}
		}
		else
		{
			// General case.
			// Assumption: cmp(a,b) emits x86 cmp a,b.
			// That means flags are for (b - a), so these jccs match RISC-V with this ordering.
			cmp(blk, rs1.host_reg, rs2.host_reg);

			switch(type)
			{
				case branch_type::BEQ:
					emit_taken_jcc(&je8);
					break;
				case branch_type::BNE:
					emit_taken_jcc(&jne8);
					break;
				case branch_type::BLT:
					emit_taken_jcc(&jl8);
					break;
				case branch_type::BGE:
					emit_taken_jcc(&jge8);
					break;
				case branch_type::BLTU:
					emit_taken_jcc(&jb8);
					break;
				case branch_type::BGEU:
					emit_taken_jcc(&jae8);
					break;
			}
		}

		// not taken
		blk.jmp_labels.push_back({ "end", blk.byte_pos, false, 1 });
		jmp8(blk, 0);

		// taken handler
		em.realize_label(blk, "taken");
		em.flush_all(blk);
		blk.jmp_labels.push_back({ "branch", blk.byte_pos, false, 4, (int64_t)blk.size + (int64_t)imm });
		jmp32(blk, 0);

		em.realize_label(blk, "slow_path");
		em.flush_all(blk);
		mov_imm64(blk, REG_RCX, pc);
		mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, exit_pc));
		blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
		jmp32(blk, 0);

		em.realize_label(blk, "end");
	},
		blk.pc + blk.size,
		&type);

	return false;
}

bool execjit_BEQ(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_branch(hart, inst, blk, emitter, branch_type::BEQ);
}

bool execjit_BNE(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_branch(hart, inst, blk, emitter, branch_type::BNE);
}

bool execjit_BLT(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_branch(hart, inst, blk, emitter, branch_type::BLT);
}

bool execjit_BGE(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_branch(hart, inst, blk, emitter, branch_type::BGE);
}

bool execjit_BLTU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_branch(hart, inst, blk, emitter, branch_type::BLTU);
}

bool execjit_BGEU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	return jit_branch(hart, inst, blk, emitter, branch_type::BGEU);
}
bool execjit_JAL(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_j_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp)
	{
		// Move PC+4 to RD
		if(rd.vreg != 0)
		{
			mov_imm64(blk, rd.host_reg, pc);
			add_r64imm8(blk, rd.host_reg, 4);
		}

		// Check for loop
		sub_mimm32(blk, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, loop_count), blk.count);
		blk.jmp_labels.push_back({ "slow_path",
								   blk.byte_pos,
								   false,
								   1 });
		js8(blk, 0);

		blk.jmp_labels.push_back({ "branch",
								   blk.byte_pos,
								   false,
								   4,
								   (int64_t)blk.size + (int64_t)imm });
		jmp32(blk, 0);
		{
			em.realize_label(blk, "slow_path");

			// Slow path, make interpreter work instead
			mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, exit_pc));
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);
		}
	}, blk.pc + blk.size);
	return true;
}
bool execjit_JALR(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		// Move PC+4 to RD
		if(rd.vreg != 0)
		{
			mov_imm64(blk, rd.host_reg, pc);
			add_r64imm8(blk, rd.host_reg, 4);
		}

		// Slow path, make interpreter work instead
		mov_imm64(blk, REG_RCX, pc);
		mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, exit_pc));
		blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
		jmp32(blk, 0);
	}, blk.pc + blk.size);
	return true;
}
bool execjit_LUI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_u_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp)
	{
		// RD = IMM << 12
		mov_imm64(blk, rd.host_reg, (int64_t)imm);
		// shl_rimm8(blk, rd.host_reg, 12); // Not doing that cuz our imm value is already offseted
	}, blk.pc + blk.size);
	return false;
}
bool execjit_AUIPC(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_u_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp)
	{
		// RD = IMM << 12
		mov_imm64(blk, rd.host_reg, (int64_t)imm);
		// shl_rimm8(blk, rd.host_reg, 12); // Not doing that cuz our imm value is already offseted
		mov_imm64(blk, REG_RCX, pc);
		add_rr(blk, rd.host_reg, REG_RCX);
	}, blk.pc + blk.size);
	return false;
}
#endif

void InstructionDecoder::init_rv64i()
{
	// R-Type
	auto inst_add  = register_instr("0000000**********000*****0110011", exec_ADD);
	auto inst_addw = register_instr("0000000**********000*****0111011", exec_ADDW);
	auto inst_sub  = register_instr("0100000**********000*****0110011", exec_SUB);
	auto inst_subw = register_instr("0100000**********000*****0111011", exec_SUBW);
	auto inst_xor  = register_instr("0000000**********100*****0110011", exec_XOR);
	auto inst_or   = register_instr("0000000**********110*****0110011", exec_OR);
	auto inst_and  = register_instr("0000000**********111*****0110011", exec_AND);
	auto inst_sll  = register_instr("0000000**********001*****0110011", exec_SLL);
	auto inst_sllw = register_instr("0000000**********001*****0111011", exec_SLLW);
	auto inst_srl  = register_instr("0000000**********101*****0110011", exec_SRL);
	auto inst_srlw = register_instr("0000000**********101*****0111011", exec_SRLW);
	auto inst_sra  = register_instr("0100000**********101*****0110011", exec_SRA);
	auto inst_sraw = register_instr("0100000**********101*****0111011", exec_SRAW);
	auto inst_slt  = register_instr("0000000**********010*****0110011", exec_SLT);
	auto inst_sltu = register_instr("0000000**********011*****0110011", exec_SLTU);

	// I-Type
	auto inst_addi	= register_instr("*****************000*****0010011", exec_ADDI, imm_I);
	auto inst_addiw = register_instr("*****************000*****0011011", exec_ADDIW, imm_I);
	auto inst_xori	= register_instr("*****************100*****0010011", exec_XORI, imm_I);
	auto inst_ori	= register_instr("*****************110*****0010011", exec_ORI, imm_I);
	auto inst_andi	= register_instr("*****************111*****0010011", exec_ANDI, imm_I);
	auto inst_slli	= register_instr("000000***********001*****0010011", exec_SLLI, shamt64);
	auto inst_slliw = register_instr("0000000**********001*****0011011", exec_SLLIW, shamt);
	auto inst_srliw = register_instr("0000000**********101*****0011011", exec_SRLIW, shamt);
	auto inst_sraiw = register_instr("0100000**********101*****0011011", exec_SRAIW, shamt);
	auto inst_srli	= register_instr("000000***********101*****0010011", exec_SRLI, shamt64);
	auto inst_srai	= register_instr("010000***********101*****0010011", exec_SRAI, shamt64);
	auto inst_slti	= register_instr("*****************010*****0010011", exec_SLTI, imm_I);
	auto inst_sltiu = register_instr("*****************011*****0010011", exec_SLTIU, imm_I);
	auto inst_lb	= register_instr("*****************000*****0000011", exec_LB, imm_I);
	auto inst_lh	= register_instr("*****************001*****0000011", exec_LH, imm_I);
	auto inst_lw	= register_instr("*****************010*****0000011", exec_LW, imm_I);
	auto inst_ld	= register_instr("*****************011*****0000011", exec_LD, imm_I);
	auto inst_lbu	= register_instr("*****************100*****0000011", exec_LBU, imm_I);
	auto inst_lhu	= register_instr("*****************101*****0000011", exec_LHU, imm_I);
	auto inst_lwu	= register_instr("*****************110*****0000011", exec_LWU, imm_I);

	// S-Type
	auto inst_sb = register_instr("*****************000*****0100011", exec_SB, imm_S);
	auto inst_sh = register_instr("*****************001*****0100011", exec_SH, imm_S);
	auto inst_sw = register_instr("*****************010*****0100011", exec_SW, imm_S);
	auto inst_sd = register_instr("*****************011*****0100011", exec_SD, imm_S);

	// B-Type
	auto inst_beq  = register_instr("*****************000*****1100011", exec_BEQ, imm_B);
	auto inst_bne  = register_instr("*****************001*****1100011", exec_BNE, imm_B);
	auto inst_blt  = register_instr("*****************100*****1100011", exec_BLT, imm_B);
	auto inst_bge  = register_instr("*****************101*****1100011", exec_BGE, imm_B);
	auto inst_bltu = register_instr("*****************110*****1100011", exec_BLTU, imm_B);
	auto inst_bgeu = register_instr("*****************111*****1100011", exec_BGEU, imm_B);

	// what
	auto inst_jal	= register_instr("*************************1101111", exec_JAL, imm_J);
	auto inst_jalr	= register_instr("*****************000*****1100111", exec_JALR, imm_I);
	auto inst_lui	= register_instr("*************************0110111", exec_LUI, imm_U);
	auto inst_auipc = register_instr("*************************0010111", exec_AUIPC, imm_U);

	register_instr("00000000000000000000000001110011", exec_ECALL);
	register_instr("00000000000100000000000001110011", exec_EBREAK);

	register_instr("0000********00000000000000001111", exec_FENCE, imm_I);

	inst_add->jit_func	 = &execjit_ADD;
	inst_addw->jit_func	 = &execjit_ADDW;
	inst_sub->jit_func	 = &execjit_SUB;
	inst_subw->jit_func	 = &execjit_SUBW;
	inst_xor->jit_func	 = &execjit_XOR;
	inst_or->jit_func	 = &execjit_OR;
	inst_and->jit_func	 = &execjit_AND;
	inst_sll->jit_func	 = &execjit_SLL;
	inst_sllw->jit_func	 = &execjit_SLLW;
	inst_srl->jit_func	 = &execjit_SRL;
	inst_srlw->jit_func	 = &execjit_SRLW;
	inst_sra->jit_func	 = &execjit_SRA;
	inst_sraw->jit_func	 = &execjit_SRAW;
	inst_slt->jit_func	 = &execjit_SLT;
	inst_sltu->jit_func	 = &execjit_SLTU;
	inst_addi->jit_func	 = &execjit_ADDI;
	inst_addiw->jit_func = &execjit_ADDIW;
	inst_xori->jit_func	 = &execjit_XORI;
	inst_ori->jit_func	 = &execjit_ORI;
	inst_andi->jit_func	 = &execjit_ANDI;
	inst_slli->jit_func	 = &execjit_SLLI;
	inst_slliw->jit_func = &execjit_SLLIW;
	inst_srli->jit_func	 = &execjit_SRLI;
	inst_srliw->jit_func = &execjit_SRLIW;
	inst_srai->jit_func	 = &execjit_SRAI;
	inst_sraiw->jit_func = &execjit_SRAIW;
	inst_slti->jit_func	 = &execjit_SLTI;
	inst_sltiu->jit_func = &execjit_SLTIU;

	inst_lb->jit_func	 = &execjit_LB;
	inst_lbu->jit_func	 = &execjit_LBU;
	inst_lh->jit_func	 = &execjit_LH;
	inst_lhu->jit_func	 = &execjit_LHU;
	inst_lw->jit_func	 = &execjit_LW;
	inst_lwu->jit_func	 = &execjit_LWU;
	inst_ld->jit_func	 = &execjit_LD;
	/*inst_sb->jit_func	 = &execjit_SB;
	inst_sh->jit_func	 = &execjit_SH;
	inst_sw->jit_func	 = &execjit_SW;
	inst_sd->jit_func	 = &execjit_SD;*/
	// inst_beq->jit_func	= &execjit_BEQ;
	// inst_bne->jit_func	= &execjit_BNE;
	// inst_blt->jit_func	= &execjit_BLT;
	// inst_bge->jit_func	= &execjit_BGE;
	// inst_bltu->jit_func	= &execjit_BLTU;
	// inst_bgeu->jit_func	= &execjit_BGEU;
	// inst_jal->jit_func	= &execjit_JAL;
	// inst_jalr->jit_func	= &execjit_JALR;
	inst_lui->jit_func	 = &execjit_LUI;
	inst_auipc->jit_func = &execjit_AUIPC;
}
