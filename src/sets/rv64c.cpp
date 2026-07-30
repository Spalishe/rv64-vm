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

using namespace rv64vm::runner;
ExecReturn exec_C_ADDI4SPN(Hart& hart, InstructionData& inst)
{
	uint8_t rd		 = d_c_rd(inst.inst);
	hart.GPR[8 + rd] = hart.GPR[2] + inst.imm;
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_LW(Hart& hart, InstructionData& inst)
{
	uint8_t rd	= d_c_rd(inst.inst);
	uint8_t rs1 = d_c_rs1(inst.inst);

	int32_t val;
	MemoryReturn success = hart.get_mmio()->read(hart, hart.GPR[8 + rs1] + inst.imm, MemorySize::Int, &val);
	if(success.is_success)
	{
		hart.GPR[8 + rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_C_LD(Hart& hart, InstructionData& inst)
{
	uint8_t rd	= d_c_rd(inst.inst);
	uint8_t rs1 = d_c_rs1(inst.inst);

	int64_t val;
	MemoryReturn success = hart.get_mmio()->read(hart, hart.GPR[8 + rs1] + inst.imm, MemorySize::Long, &val);
	if(success.is_success)
	{
		hart.GPR[8 + rd] = val;
	}
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#ifdef USE_FPU
ExecReturn exec_C_FLD(Hart& hart, InstructionData& inst)
{
	uint8_t rd	= d_c_rd(inst.inst);
	uint8_t rs1 = d_c_rs1(inst.inst);

	int64_t val;
	MemoryReturn success = hart.get_mmio()->read(hart, hart.GPR[8 + rs1] + inst.imm, MemorySize::Long, &val);
	if(success.is_success)
	{
		hart.FPR[8 + rd] = std::bit_cast<double>(val);
	}
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#endif
ExecReturn exec_C_SW(Hart& hart, InstructionData& inst)
{
	uint8_t rs2 = d_c_rd(inst.inst);
	uint8_t rs1 = d_c_rs1(inst.inst);

	MemoryReturn success = hart.get_mmio()->write(hart, hart.GPR[8 + rs1] + inst.imm, MemorySize::Int, hart.GPR[8 + rs2]);
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_C_SD(Hart& hart, InstructionData& inst)
{
	uint8_t rs2 = d_c_rd(inst.inst);
	uint8_t rs1 = d_c_rs1(inst.inst);

	MemoryReturn success = hart.get_mmio()->write(hart, hart.GPR[8 + rs1] + inst.imm, MemorySize::Long, hart.GPR[8 + rs2]);
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#ifdef USE_FPU
ExecReturn exec_C_FSD(Hart& hart, InstructionData& inst)
{
	uint8_t rs2 = d_c_rd(inst.inst);
	uint8_t rs1 = d_c_rs1(inst.inst);

	MemoryReturn success = hart.get_mmio()->write(hart, hart.GPR[8 + rs1] + inst.imm, MemorySize::Long, std::bit_cast<uint64_t>(hart.FPR[8 + rs2]));
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#endif
ExecReturn exec_C_NOP(Hart& hart, InstructionData& inst)
{
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_ADDI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] += inst.imm;
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_ADDIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)(hart.GPR[inst.rd] + inst.imm);
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_LI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = inst.imm;
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_LUI_ADDI16SP(Hart& hart, InstructionData& inst)
{
	if(inst.rd == 2)
	{
		// C.ADDI16SP
		hart.GPR[2] += sext(d_c_nzimm_9(inst.inst), 10);
	}
	else
	{
		// C.LUI
		hart.GPR[inst.rd] = inst.imm << 12;
	}
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_SRLI(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + inst.rd] >>= inst.imm;
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_SRAI(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] = (int64_t)hart.GPR[8 + (inst.rd & 0x7)] >> inst.imm;
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_ANDI(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] &= (int32_t)sext(inst.imm, 6);
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_SUB(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] -= hart.GPR[8 + d_c_rd(inst.inst)];
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_XOR(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] ^= hart.GPR[8 + d_c_rd(inst.inst)];
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_OR(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] |= hart.GPR[8 + d_c_rd(inst.inst)];
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_AND(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] &= hart.GPR[8 + d_c_rd(inst.inst)];
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_SUBW(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] = (int32_t)(hart.GPR[8 + (inst.rd & 0x7)] - hart.GPR[8 + d_c_rd(inst.inst)]);
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_ADDW(Hart& hart, InstructionData& inst)
{
	hart.GPR[8 + (inst.rd & 0x7)] = (int32_t)(hart.GPR[8 + (inst.rd & 0x7)] + hart.GPR[8 + d_c_rd(inst.inst)]);
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_J(Hart& hart, InstructionData& inst)
{
	hart.pc += (int32_t)inst.imm;
	return { true, true, 0, 0, 0 };
}
ExecReturn exec_C_BEQZ(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[8 + (inst.rd & 0x7)] == 0)
	{
		hart.pc += (int32_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_BNEZ(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[8 + (inst.rd & 0x7)] != 0)
	{
		hart.pc += (int32_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_SLLI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] <<= inst.imm;
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_LWSP(Hart& hart, InstructionData& inst)
{
	int32_t val;
	MemoryReturn success = hart.get_mmio()->read(hart, hart.GPR[2] + inst.imm, MemorySize::Int, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_C_LDSP(Hart& hart, InstructionData& inst)
{
	int64_t val;
	MemoryReturn success = hart.get_mmio()->read(hart, hart.GPR[2] + inst.imm, MemorySize::Long, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#ifdef USE_FPU
ExecReturn exec_C_FLDSP(Hart& hart, InstructionData& inst)
{
	int64_t val;
	MemoryReturn success = hart.get_mmio()->read(hart, hart.GPR[2] + inst.imm, MemorySize::Long, &val);
	if(success.is_success)
	{
		hart.FPR[inst.rd] = std::bit_cast<double>((uint64_t)val);
	}
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#endif
ExecReturn exec_C_JR(Hart& hart, InstructionData& inst)
{
	hart.pc = hart.GPR[inst.rd] & ~1ULL;
	return { true, true, 0, 0, 0 };
}
ExecReturn exec_C_MV(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[d_c_rs2(inst.inst)];
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_EBREAK(Hart& hart, InstructionData& inst)
{
	return { false, true, 0, EXC_BREAKPOINT, hart.pc };
}
ExecReturn exec_C_JALR(Hart& hart, InstructionData& inst)
{
	uint64_t t = hart.pc + 2;
	hart.pc	   = hart.GPR[inst.rd] & ~1ULL;
	;
	hart.GPR[1] = t;
	return { true, true, 0, 0, 0 };
}
ExecReturn exec_C_ADD(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] += hart.GPR[d_c_rs2(inst.inst)];
	return { true, false, 2, 0, 0 };
}
ExecReturn exec_C_SWSP(Hart& hart, InstructionData& inst)
{
	uint8_t rs2 = d_c_rs2(inst.inst);

	MemoryReturn success = hart.get_mmio()->write(hart, hart.GPR[2] + inst.imm, MemorySize::Int, hart.GPR[rs2]);
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_C_SDSP(Hart& hart, InstructionData& inst)
{
	uint8_t rs2 = d_c_rs2(inst.inst);

	MemoryReturn success = hart.get_mmio()->write(hart, hart.GPR[2] + inst.imm, MemorySize::Long, hart.GPR[rs2]);
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#ifdef USE_FPU
ExecReturn exec_C_FSDSP(Hart& hart, InstructionData& inst)
{
	uint8_t rs2 = d_c_rs2(inst.inst);

	MemoryReturn success = hart.get_mmio()->write(hart, hart.GPR[2] + inst.imm, MemorySize::Long, std::bit_cast<uint64_t>(hart.FPR[rs2]));
	return {
		success.is_success,
		false,
		2,
		success.exc_code,
		success.tval,
	};
}
#endif

void InstructionDecoder::init_rv64c()
{
	register_instr("000***********00", exec_C_ADDI4SPN, d_c_uimm);
	register_instr("010***********00", exec_C_LW, d_c_uimm_cl);
	register_instr("011***********00", exec_C_LD, d_c_uimm_cl1);
	register_instr("110***********00", exec_C_SW, d_c_uimm_cl);
	register_instr("111***********00", exec_C_SD, d_c_uimm_cl1);
	register_instr("0000000000000001", exec_C_NOP);
	register_instr("000***********01", exec_C_ADDI, d_c_nzimm);
	// register_instr("001***********01", exec_C_JAL); // Reserved for RV32
	register_instr("001***********01", exec_C_ADDIW, d_c_nzimm);
	register_instr("010***********01", exec_C_LI, d_c_nzimm);
	register_instr("011***********01", exec_C_LUI_ADDI16SP, d_c_nzimm); // Differs with RD: If rd == 2, then it ADDI16SP, else LUI
	register_instr("100*00********01", exec_C_SRLI, d_c_uimm_arith);
	register_instr("100*01********01", exec_C_SRAI, d_c_uimm_arith);
	register_instr("100*10********01", exec_C_ANDI, d_c_uimm_arith);
	register_instr("100011***00***01", exec_C_SUB);
	register_instr("100011***01***01", exec_C_XOR);
	register_instr("100011***10***01", exec_C_OR);
	register_instr("100011***11***01", exec_C_AND);
	register_instr("100111***00***01", exec_C_SUBW);
	register_instr("100111***01***01", exec_C_ADDW);
	register_instr("101***********01", exec_C_J, d_c_j_imm);
	register_instr("110***********01", exec_C_BEQZ, d_c_b_imm);
	register_instr("111***********01", exec_C_BNEZ, d_c_b_imm);
	register_instr("000***********10", exec_C_SLLI, d_c_uimm_arith);
	register_instr("010***********10", exec_C_LWSP, d_c_uimm_lwsp);
	register_instr("011***********10", exec_C_LDSP, d_c_uimm_ldsp);
	register_instr("1000*****0000010", exec_C_JR);
	register_instr("1000**********10", exec_C_MV);
	register_instr("1001000000000010", exec_C_EBREAK);
	register_instr("1001*****0000010", exec_C_JALR);
	register_instr("1001**********10", exec_C_ADD);
	register_instr("110***********10", exec_C_SWSP, d_c_uimm_swsp);
	register_instr("111***********10", exec_C_SDSP, d_c_uimm_sdsp);
#ifdef USE_FPU
	// Note: only RV64DC are made. If you planning to make RV32, make sure to make C.FSW, C.FLW, etc.
	register_instr("001***********00", exec_C_FLD, d_c_uimm_cl1);
	register_instr("101***********00", exec_C_FSD, d_c_uimm_cl1);
	register_instr("001***********10", exec_C_FLDSP, d_c_uimm_ldsp);
	register_instr("101***********10", exec_C_FSDSP, d_c_uimm_sdsp);

#endif
}
