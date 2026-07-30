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
ExecReturn exec_CLMUL(Hart& hart, InstructionData& inst)
{
	uint64_t rs1 = hart.GPR[inst.rs1];
	uint64_t rs2 = hart.GPR[inst.rs2];
	uint64_t out = 0;

	for(int i = 0; i < 64; i++)
	{
		out = ((rs2 >> i) & 1) ? (out ^ (rs1 << i)) : out;
	}

	hart.GPR[inst.rd] = out;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CLMULH(Hart& hart, InstructionData& inst)
{
	uint64_t rs1 = hart.GPR[inst.rs1];
	uint64_t rs2 = hart.GPR[inst.rs2];
	uint64_t out = 0;

	for(int i = 1; i < 64; i++)
	{
		out = ((rs2 >> i) & 1) ? (out ^ (rs1 >> (64 - i))) : out;
	}

	hart.GPR[inst.rd] = out;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CLMULR(Hart& hart, InstructionData& inst)
{
	uint64_t rs1 = hart.GPR[inst.rs1];
	uint64_t rs2 = hart.GPR[inst.rs2];
	uint64_t out = 0;

	for(int i = 0; i < 64; i++)
	{
		out = ((rs2 >> i) & 1) ? (out ^ (rs1 >> (63 - i))) : out;
	}

	hart.GPR[inst.rd] = out;
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zbc()
{
	register_instr("0000101**********001*****0110011", exec_CLMUL);
	register_instr("0000101**********011*****0110011", exec_CLMULH);
	register_instr("0000101**********010*****0110011", exec_CLMULR);
}
