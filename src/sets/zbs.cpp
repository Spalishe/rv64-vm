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
ExecReturn exec_BCLR(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = hart.GPR[inst.rs2] & 0x3f;
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & ~((uint64_t)1 << index);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BCLRI(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = inst.imm & 0x3f;
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & ~((uint64_t)1 << index);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_BEXT(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = hart.GPR[inst.rs2] & 0x3f;
	hart.GPR[inst.rd] = (hart.GPR[inst.rs1] >> index) & 1;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BEXTI(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = inst.imm & 0x3f;
	hart.GPR[inst.rd] = (hart.GPR[inst.rs1] >> index) & 1;
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_BINV(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = hart.GPR[inst.rs2] & 0x3f;
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] ^ ((uint64_t)1 << index);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BINVI(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = inst.imm & 0x3f;
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] ^ ((uint64_t)1 << index);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_BSET(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = hart.GPR[inst.rs2] & 0x3f;
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] | ((uint64_t)1 << index);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BSETI(Hart& hart, InstructionData& inst)
{
	uint64_t index	  = inst.imm & 0x3f;
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] | ((uint64_t)1 << index);
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zbs()
{
	register_instr("0100100**********001*****0110011", exec_BCLR);
	register_instr("010010***********001*****0010011", exec_BCLRI);
	register_instr("0100100**********101*****0110011", exec_BEXT);
	register_instr("010010***********101*****0010011", exec_BEXTI);
	register_instr("0110100**********001*****0110011", exec_BINV);
	register_instr("011010***********001*****0010011", exec_BINVI);
	register_instr("0010100**********001*****0110011", exec_BSET);
	register_instr("001010***********001*****0010011", exec_BSETI);
}
