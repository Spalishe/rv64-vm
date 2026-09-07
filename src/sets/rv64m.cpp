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
#ifdef USE_JIT
#include "../../include/jit/rvjit.hpp"
#endif

#include <limits>
using namespace rv64vm::runner;

// R-Type

ExecReturn exec_MULW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (uint64_t)((int32_t)hart.GPR[inst.rs1] * (int32_t)hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_DIVW(Hart& hart, InstructionData& inst)
{
	if((int32_t)hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)-1;
	}
	else if((int32_t)hart.GPR[inst.rs1] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)0;
	}
	else if((int32_t)hart.GPR[inst.rs1] == std::numeric_limits<int32_t>::min() && (int32_t)hart.GPR[inst.rs2] == -1)
	{
		hart.GPR[inst.rd] = (int32_t)hart.GPR[inst.rs1];
	}
	else
	{
		hart.GPR[inst.rd] = (uint64_t)((int32_t)hart.GPR[inst.rs1] / (int32_t)hart.GPR[inst.rs2]);
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_DIVUW(Hart& hart, InstructionData& inst)
{
	if((uint32_t)hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)-1;
	}
	else if((uint32_t)hart.GPR[inst.rs1] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)0;
	}
	else
	{
		hart.GPR[inst.rd] = (uint64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] / (uint32_t)hart.GPR[inst.rs2]);
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_REMW(Hart& hart, InstructionData& inst)
{
	if((int32_t)hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)(int32_t)hart.GPR[inst.rs1];
	}
	else if((int32_t)hart.GPR[inst.rs1] == std::numeric_limits<int32_t>::min() && (int32_t)hart.GPR[inst.rs2] == -1)
	{
		hart.GPR[inst.rd] = 0;
	}
	else
	{
		hart.GPR[inst.rd] = (uint64_t)(int64_t)((int32_t)hart.GPR[inst.rs1] % (int32_t)hart.GPR[inst.rs2]);
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_REMUW(Hart& hart, InstructionData& inst)
{
	if((uint32_t)hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = hart.GPR[inst.rs1];
	}
	else
	{
		hart.GPR[inst.rd] = (uint64_t)(int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] % (uint32_t)hart.GPR[inst.rs2]);
	}
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_MUL(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] * hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_MULH(Hart& hart, InstructionData& inst)
{
	__int128_t res	  = (__int128_t)(int64_t)hart.GPR[inst.rs1] * (__int128_t)(int64_t)hart.GPR[inst.rs2];
	hart.GPR[inst.rd] = (uint64_t)(res >> 64);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_MULHSU(Hart& hart, InstructionData& inst)
{
	__uint128_t res	  = (__uint128_t)(__int128_t)(int64_t)hart.GPR[inst.rs1] * (__uint128_t)hart.GPR[inst.rs2];
	hart.GPR[inst.rd] = (uint64_t)(res >> 64);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_MULHU(Hart& hart, InstructionData& inst)
{
	__uint128_t res	  = (__uint128_t)hart.GPR[inst.rs1] * (__uint128_t)hart.GPR[inst.rs2];
	hart.GPR[inst.rd] = (uint64_t)(res >> 64);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_DIV(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)-1;
	}
	else if(hart.GPR[inst.rs1] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)0;
	}
	else if(hart.GPR[inst.rs1] == std::numeric_limits<int64_t>::min() && hart.GPR[inst.rs2] == -1)
	{
		hart.GPR[inst.rd] = hart.GPR[inst.rs1];
	}
	else
	{
		hart.GPR[inst.rd] = (uint64_t)((int64_t)hart.GPR[inst.rs1] / (int64_t)hart.GPR[inst.rs2]);
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_DIVU(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)-1;
	}
	else if(hart.GPR[inst.rs1] == 0)
	{
		hart.GPR[inst.rd] = (uint64_t)0;
	}
	else
	{
		hart.GPR[inst.rd] = hart.GPR[inst.rs1] / hart.GPR[inst.rs2];
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_REM(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = hart.GPR[inst.rs1];
	}
	else if(hart.GPR[inst.rs1] == std::numeric_limits<int64_t>::min() && hart.GPR[inst.rs2] == -1)
	{
		hart.GPR[inst.rd] = 0;
	}
	else
	{
		hart.GPR[inst.rd] = (uint64_t)((int64_t)hart.GPR[inst.rs1] % (int64_t)hart.GPR[inst.rs2]);
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_REMU(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs2] == 0)
	{
		hart.GPR[inst.rd] = hart.GPR[inst.rs1];
	}
	else
	{
		hart.GPR[inst.rd] = hart.GPR[inst.rs1] % (int64_t)hart.GPR[inst.rs2];
	}
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_rv64m()
{
	auto inst_mul   = register_instr("0000001**********000*****0110011", exec_MUL);
	auto inst_mulw  = register_instr("0000001**********000*****0111011", exec_MULW);
	auto inst_mulh  = register_instr("0000001**********001*****0110011", exec_MULH);
	auto inst_mulhsu = register_instr("0000001**********010*****0110011", exec_MULHSU);
	auto inst_mulhu = register_instr("0000001**********011*****0110011", exec_MULHU);
	auto inst_div   = register_instr("0000001**********100*****0110011", exec_DIV);
	auto inst_divu  = register_instr("0000001**********101*****0110011", exec_DIVU);
	auto inst_divuw = register_instr("0000001**********101*****0111011", exec_DIVUW);
	auto inst_divw  = register_instr("0000001**********100*****0111011", exec_DIVW);
	auto inst_rem   = register_instr("0000001**********110*****0110011", exec_REM);
	auto inst_remu  = register_instr("0000001**********111*****0110011", exec_REMU);
	auto inst_remuw = register_instr("0000001**********111*****0111011", exec_REMUW);
	auto inst_remw  = register_instr("0000001**********110*****0111011", exec_REMW);

#ifdef USE_JIT
	inst_mul->jit_func	  = &jit::jit_MUL;
	inst_mulw->jit_func	 = &jit::jit_MULW;
	inst_mulh->jit_func	 = &jit::jit_MULH;
	inst_mulhsu->jit_func = &jit::jit_MULHSU;
	inst_mulhu->jit_func	 = &jit::jit_MULHU;
	inst_div->jit_func	  = &jit::jit_DIV;
	inst_divu->jit_func	 = &jit::jit_DIVU;
	inst_divuw->jit_func = &jit::jit_DIVUW;
	inst_divw->jit_func	 = &jit::jit_DIVW;
	inst_rem->jit_func	  = &jit::jit_REM;
	inst_remu->jit_func	 = &jit::jit_REMU;
	inst_remuw->jit_func = &jit::jit_REMUW;
	inst_remw->jit_func	 = &jit::jit_REMW;
#endif
}
