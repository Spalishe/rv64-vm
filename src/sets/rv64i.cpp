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

using namespace rv64vm::runner;
using PrivilegeMode = Hart::PrivilegeMode;
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
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Long, &val);

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
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Int, &val);

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
	MemoryReturn out = hart.get_mmio()->write(hart, addr, MemorySize::Long, hart.GPR[inst.rs2]);
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
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Byte, &val);
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
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Short, &val);
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
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Int, &val);
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
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Byte, &val);
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
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Short, &val);

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
	MemoryReturn out = hart.get_mmio()->write(hart, addr, MemorySize::Byte, hart.GPR[inst.rs2]);
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
	MemoryReturn out = hart.get_mmio()->write(hart, addr, MemorySize::Short, hart.GPR[inst.rs2]);
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
	MemoryReturn out = hart.get_mmio()->write(hart, addr, MemorySize::Int, hart.GPR[inst.rs2]);
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
}
