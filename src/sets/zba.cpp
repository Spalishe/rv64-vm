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
ExecReturn exec_ADD_UW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs2] + (uint32_t)hart.GPR[inst.rs1];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SH1ADD(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs2] + (hart.GPR[inst.rs1] << 1);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SH1ADD_UW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs2] + ((uint64_t)(uint32_t)hart.GPR[inst.rs1] << 1);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SH2ADD(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs2] + (hart.GPR[inst.rs1] << 2);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SH2ADD_UW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs2] + ((uint64_t)(uint32_t)hart.GPR[inst.rs1] << 2);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SH3ADD(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs2] + (hart.GPR[inst.rs1] << 3);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SH3ADD_UW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs2] + ((uint64_t)(uint32_t)hart.GPR[inst.rs1] << 3);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLLI_UW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((uint64_t)(uint32_t)hart.GPR[inst.rs1]) << inst.imm;
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zba()
{
	register_instr("0000100**********000*****0111011", exec_ADD_UW);
	register_instr("0010000**********010*****0110011", exec_SH1ADD);
	register_instr("0010000**********010*****0111011", exec_SH1ADD_UW);
	register_instr("0010000**********100*****0110011", exec_SH2ADD);
	register_instr("0010000**********100*****0111011", exec_SH2ADD_UW);
	register_instr("0010000**********110*****0110011", exec_SH3ADD);
	register_instr("0010000**********110*****0111011", exec_SH3ADD_UW);
	register_instr("000010***********001*****0011011", exec_SLLI_UW);
}
