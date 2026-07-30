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
ExecReturn exec_ANDN(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & ~hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ORN(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] | ~hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_XNOR(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ~(hart.GPR[inst.rs1] ^ hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CLZ(Hart& hart, InstructionData& inst)
{
	uint64_t c = -1;
	for(int i = 63; i >= 0; i--)
	{
		uint8_t bit = (hart.GPR[inst.rs1] >> i) & 1;
		if(bit)
		{
			c = i;
			break;
		}
	}
	hart.GPR[inst.rd] = 63 - c;

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CLZW(Hart& hart, InstructionData& inst)
{
	uint64_t c = -1;
	for(int i = 31; i >= 0; i--)
	{
		uint8_t bit = ((uint32_t)hart.GPR[inst.rs1] >> i) & 1;
		if(bit)
		{
			c = i;
			break;
		}
	}
	hart.GPR[inst.rd] = 31 - c;

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CTZ(Hart& hart, InstructionData& inst)
{
	uint64_t c = 64;
	for(int i = 0; i < 64; i++)
	{
		uint8_t bit = (hart.GPR[inst.rs1] >> i) & 1;
		if(bit)
		{
			c = i;
			break;
		}
	}
	hart.GPR[inst.rd] = c;

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CTZW(Hart& hart, InstructionData& inst)
{
	uint64_t c = 32;
	for(int i = 0; i < 32; i++)
	{
		uint8_t bit = ((uint32_t)hart.GPR[inst.rs1] >> i) & 1;
		if(bit)
		{
			c = i;
			break;
		}
	}
	hart.GPR[inst.rd] = c;

	return { true, false, 4, 0, 0 };
}

ExecReturn exec_CPOP(Hart& hart, InstructionData& inst)
{
	uint8_t bitcount = 0;
	for(int i = 0; i < 64; i++)
	{
		uint8_t bit = (hart.GPR[inst.rs1] >> i) & 1;
		bitcount += bit;
	}
	hart.GPR[inst.rd] = bitcount;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CPOPW(Hart& hart, InstructionData& inst)
{
	uint8_t bitcount = 0;
	for(int i = 0; i < 32; i++)
	{
		uint8_t bit = ((uint32_t)hart.GPR[inst.rs1] >> i) & 1;
		bitcount += bit;
	}
	hart.GPR[inst.rd] = bitcount;
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_MAX(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = std::max((int64_t)hart.GPR[inst.rs1], (int64_t)hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_MAXU(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = std::max(hart.GPR[inst.rs1], hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_MIN(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = std::min((int64_t)hart.GPR[inst.rs1], (int64_t)hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_MINU(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = std::min(hart.GPR[inst.rs1], hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_SEXT_B(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int8_t)hart.GPR[inst.rs1];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SEXT_H(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int16_t)hart.GPR[inst.rs1];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ZEXT_H(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & 0xFFFF;
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_ROL(Hart& hart, InstructionData& inst)
{
	uint64_t rs1	  = hart.GPR[inst.rs1];
	uint64_t shamt	  = hart.GPR[inst.rs2] & 0x3F;
	hart.GPR[inst.rd] = (rs1 << shamt) | (rs1 >> (64 - shamt));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ROLW(Hart& hart, InstructionData& inst)
{
	uint64_t rs1	  = (uint32_t)hart.GPR[inst.rs1];
	uint64_t shamt	  = hart.GPR[inst.rs2] & 0x1F;
	hart.GPR[inst.rd] = (uint64_t)(int64_t)(int32_t)((rs1 << shamt) | (rs1 >> (32 - shamt)));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ROR(Hart& hart, InstructionData& inst)
{
	uint64_t rs1	  = hart.GPR[inst.rs1];
	uint64_t shamt	  = hart.GPR[inst.rs2] & 0x3F;
	hart.GPR[inst.rd] = (rs1 >> shamt) | (rs1 << (64 - shamt));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_RORI(Hart& hart, InstructionData& inst)
{
	uint64_t rs1	  = hart.GPR[inst.rs1];
	hart.GPR[inst.rd] = (rs1 >> inst.imm) | (rs1 << (64 - inst.imm));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_RORIW(Hart& hart, InstructionData& inst)
{
	uint64_t rs1	  = (uint32_t)hart.GPR[inst.rs1];
	hart.GPR[inst.rd] = (rs1 >> inst.imm) | (rs1 << (32 - inst.imm));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_RORW(Hart& hart, InstructionData& inst)
{
	uint64_t rs1	  = (uint32_t)hart.GPR[inst.rs1];
	uint64_t shamt	  = hart.GPR[inst.rs2] & 0x1F;
	hart.GPR[inst.rd] = (uint64_t)(int64_t)(int32_t)((rs1 >> shamt) | (rs1 << (32 - shamt)));
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_ORC_B(Hart& hart, InstructionData& inst)
{
	uint64_t in	 = hart.GPR[inst.rs1];
	uint64_t out = 0;
	for(int i = 0; i < 64; i += 8)
	{
		uint8_t b	 = (in >> i) & 0xFF;
		uint64_t val = (b == 0) ? 0x00 : 0xFF;
		out |= (val << i);
	}
	hart.GPR[inst.rd] = out;
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_REV8(Hart& hart, InstructionData& inst)
{
	uint64_t in	 = hart.GPR[inst.rs1];
	uint64_t out = 0;
	for(int i = 0; i < 64 / 8; i++)
	{
		out |= ((in >> (i * 8)) & 0xFF) << ((64 / 8 - 1 - i) * 8);
	}
	hart.GPR[inst.rd] = out;
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zbb()
{
	register_instr("0100000**********111*****0110011", exec_ANDN);
	register_instr("0100000**********110*****0110011", exec_ORN);
	register_instr("0100000**********100*****0110011", exec_XNOR);
	register_instr("011000000000*****001*****0010011", exec_CLZ);
	register_instr("011000000000*****001*****0011011", exec_CLZW);
	register_instr("011000000001*****001*****0010011", exec_CTZ);
	register_instr("011000000001*****001*****0011011", exec_CTZW);
	register_instr("011000000010*****001*****0010011", exec_CPOP);
	register_instr("011000000010*****001*****0011011", exec_CPOPW);
	register_instr("0000101**********110*****0110011", exec_MAX);
	register_instr("0000101**********111*****0110011", exec_MAXU);
	register_instr("0000101**********100*****0110011", exec_MIN);
	register_instr("0000101**********101*****0110011", exec_MINU);
	register_instr("011000000100*****001*****0010011", exec_SEXT_B);
	register_instr("011000000101*****001*****0010011", exec_SEXT_H);
	register_instr("000010000000*****100*****0111011", exec_ZEXT_H); // OP-32 If emulator running in 64-bit mode
	register_instr("0110000**********001*****0110011", exec_ROL);
	register_instr("0110000**********001*****0111011", exec_ROLW);
	register_instr("0110000**********101*****0110011", exec_ROR);
	register_instr("011000***********101*****0010011", exec_RORI);
	register_instr("0110000**********101*****0011011", exec_RORIW);
	register_instr("0110000**********101*****0111011", exec_RORW);
	register_instr("001010000111*****101*****0010011", exec_ORC_B);
	register_instr("011010111000*****101*****0010011", exec_REV8);
	//                          ^ RV64 arch bit, in RV32 set to 0
}
