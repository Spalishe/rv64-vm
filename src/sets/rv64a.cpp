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

using namespace rv64vm::runner;
MemoryReturn AMO_SC(Hart& hart, uint64_t va, MemorySize size, uint64_t val, void* out_val)
{
	uint64_t pa;
	auto ret = hart.get_mmu().translate(&hart, AccessType::STORE, va, &pa);
	if(!ret.is_success) return ret;

	if(hart.get_reservation().valid && hart.get_reservation().paddr == pa && hart.get_reservation().size == size)
	{
		MemoryReturn out = hart.get_mmio()->write(hart, pa, size, val, true);
		if(!out.is_success) return out;
		// amo_check_reservation(hart, va);
		hart.get_reservation().valid = false;
		*(uint8_t*)out_val			 = 0;
		return out;
	}
	else
	{
		hart.get_reservation().valid = false;
		*(uint8_t*)out_val			 = 1;
		return { true, 0, 0 };
	}
}
MemoryReturn AMO_LR(Hart& hart, uint64_t va, MemorySize size, void* val)
{
	uint64_t pa;
	auto ret = hart.get_mmu().translate(&hart, AccessType::LOAD, va, &pa);
	if(!ret.is_success) return ret;

	uint64_t value;
	MemoryReturn p = hart.get_mmio()->read(hart, pa, size, &value, true);
	if(!p.is_success) return p;
	hart.get_reservation().valid = true;
	hart.get_reservation().size	 = size;
	hart.get_reservation().paddr = pa;
	switch(size)
	{
		case MemorySize::Byte:
			*(uint8_t*)val = value;
			break;
		case MemorySize::Short:
			*(uint16_t*)val = value;
			break;
		case MemorySize::Int:
			*(uint32_t*)val = value;
			break;
		case MemorySize::Long:
			*(uint64_t*)val = value;
			break;
	}
	return p;
}

ExecReturn exec_LR_D(Hart& hart, InstructionData& inst)
{
	uint64_t val	 = 0;
	MemoryReturn out = AMO_LR(hart, hart.GPR[inst.rs1], MemorySize::Long, &val);
	if(!out.is_success) return { false, false, 4, out.exc_code, out.tval };
	hart.GPR[inst.rd] = val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SC_D(Hart& hart, InstructionData& inst)
{
	uint64_t val	 = 0;
	MemoryReturn out = AMO_SC(hart, hart.GPR[inst.rs1], MemorySize::Long, hart.GPR[inst.rs2], &val);
	if(!out.is_success) return { false, false, 4, out.exc_code, out.tval };
	hart.GPR[inst.rd] = val;
	return { true, false, 4, 0, 0 };
}

MemoryReturn AMO64(Hart& hart, uint64_t va, uint64_t rs2, uint64_t (*func)(uint64_t a, uint64_t b), uint64_t* out_val)
{
	uint64_t pa;

	auto ret = hart.get_mmu().translate(&hart, AccessType::STORE, va, &pa);

	if(!ret.is_success)
		return ret;

	uint64_t val;
	MemoryReturn out = hart.get_mmio()->read(hart, pa, MemorySize::Long, &val, true);
	if(!out.is_success) return out;
	uint64_t new_val = func(val, rs2);

	MemoryReturn s = hart.get_mmio()->write(hart, pa, MemorySize::Long, new_val, true);
	if(!s.is_success) return s;
	*(uint64_t*)out_val = val;
	return { true, 0, 0 };
}

ExecReturn exec_AMOSWAP_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOADD_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return a + b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOXOR_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return a ^ b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOAND_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return a & b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOOR_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return a | b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMIN_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return (uint64_t)std::min((int64_t)a, (int64_t)b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMAX_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return (uint64_t)std::max((int64_t)a, (int64_t)b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMINU_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return std::min(a, b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMAXU_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO64(hart, hart.GPR[inst.rs1], hart.GPR[inst.rs2], [](uint64_t a, uint64_t b)
	{ return std::max(a, b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}

// RV32A

ExecReturn exec_LR_W(Hart& hart, InstructionData& inst)
{
	uint32_t val = 0;
	MemoryReturn out = AMO_LR(hart, hart.GPR[inst.rs1], MemorySize::Int, &val);
	if(!out.is_success) return { false, false, 0, out.exc_code, out.tval };
	hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SC_W(Hart& hart, InstructionData& inst)
{
	uint32_t val = 0;
	MemoryReturn out = AMO_SC(hart, hart.GPR[inst.rs1], MemorySize::Int, (uint32_t)hart.GPR[inst.rs2], &val);
	if(!out.is_success) return { false, false, 0, out.exc_code, out.tval };
	hart.GPR[inst.rd] = val;
	return { true, false, 4, 0, 0 };
}

MemoryReturn AMO32(Hart& hart, uint64_t va, uint32_t rs2, uint32_t (*func)(uint32_t a, uint32_t b), uint32_t* out_val)
{
	uint64_t pa;

	auto ret = hart.get_mmu().translate(&hart, AccessType::STORE, va, &pa);

	if(!ret.is_success)
		return ret;
	uint32_t val;
	MemoryReturn out = hart.get_mmio()->read(hart, pa, MemorySize::Int, &val, true);
	if(!out.is_success) return out;
	uint64_t new_val = func(val, rs2);

	MemoryReturn s = hart.get_mmio()->write(hart, pa, MemorySize::Int, new_val, true);
	if(!s.is_success) return s;
	*(uint32_t*)out_val = val;
	return { true, 0, 0 };
}

ExecReturn exec_AMOSWAP_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOADD_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return a + b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOXOR_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return a ^ b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOAND_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return a & b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOOR_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return a | b; }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMIN_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return (uint32_t)std::min((int32_t)a, (int32_t)b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMAX_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return (uint32_t)std::max((int32_t)a, (int32_t)b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMINU_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return std::min(a, b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}
ExecReturn exec_AMOMAXU_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO32(hart, hart.GPR[inst.rs1], (uint32_t)hart.GPR[inst.rs2], [](uint32_t a, uint32_t b)
	{ return std::max(a, b); }, &val);
	if(out.is_success) hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { out.is_success, false, 4, out.exc_code, out.tval };
}

void InstructionDecoder::init_rv64a()
{
	auto inst_amoswap_w = register_instr("00001************010*****0101111", exec_AMOSWAP_W);
	auto inst_amoadd_w	= register_instr("00000************010*****0101111", exec_AMOADD_W);
	auto inst_amoand_w	= register_instr("01100************010*****0101111", exec_AMOAND_W);
	auto inst_amoxor_w	= register_instr("00100************010*****0101111", exec_AMOXOR_W);
	auto inst_amoor_w	= register_instr("01000************010*****0101111", exec_AMOOR_W);
	auto inst_amomin_w	= register_instr("10000************010*****0101111", exec_AMOMIN_W);
	auto inst_amominu_w = register_instr("11000************010*****0101111", exec_AMOMINU_W);
	auto inst_amomax_w	= register_instr("10100************010*****0101111", exec_AMOMAX_W);
	auto inst_amomaxu_w = register_instr("11100************010*****0101111", exec_AMOMAXU_W);
	auto inst_lr_w		= register_instr("00010************010*****0101111", exec_LR_W);
	auto inst_sc_w		= register_instr("00011************010*****0101111", exec_SC_W);

	auto inst_amoswap_d = register_instr("00001************011*****0101111", exec_AMOSWAP_D);
	auto inst_amoadd_d	= register_instr("00000************011*****0101111", exec_AMOADD_D);
	auto inst_amoand_d	= register_instr("01100************011*****0101111", exec_AMOAND_D);
	auto inst_amoxor_d	= register_instr("00100************011*****0101111", exec_AMOXOR_D);
	auto inst_amoor_d	= register_instr("01000************011*****0101111", exec_AMOOR_D);
	auto inst_amomin_d	= register_instr("10000************011*****0101111", exec_AMOMIN_D);
	auto inst_amominu_d = register_instr("11000************011*****0101111", exec_AMOMINU_D);
	auto inst_amomax_d	= register_instr("10100************011*****0101111", exec_AMOMAX_D);
	auto inst_amomaxu_d = register_instr("11100************011*****0101111", exec_AMOMAXU_D);
	auto inst_lr_d		= register_instr("00010************011*****0101111", exec_LR_D);
	auto inst_sc_d		= register_instr("00011************011*****0101111", exec_SC_D);

#ifdef USE_JIT
	inst_lr_w->jit_func		 = &jit::jit_LR_W;
	inst_sc_w->jit_func		 = &jit::jit_SC_W;
	inst_amoswap_w->jit_func = &jit::jit_AMOSWAP_W;
	inst_amoadd_w->jit_func	 = &jit::jit_AMOADD_W;
	inst_amoxor_w->jit_func	 = &jit::jit_AMOXOR_W;
	inst_amoand_w->jit_func	 = &jit::jit_AMOAND_W;
	inst_amoor_w->jit_func	 = &jit::jit_AMOOR_W;
	inst_amomin_w->jit_func	 = &jit::jit_AMOMIN_W;
	inst_amomax_w->jit_func	 = &jit::jit_AMOMAX_W;
	inst_amominu_w->jit_func = &jit::jit_AMOMINU_W;
	inst_amomaxu_w->jit_func = &jit::jit_AMOMAXU_W;

	inst_lr_d->jit_func		 = &jit::jit_LR_D;
	inst_sc_d->jit_func		 = &jit::jit_SC_D;
	inst_amoswap_d->jit_func = &jit::jit_AMOSWAP_D;
	inst_amoadd_d->jit_func	 = &jit::jit_AMOADD_D;
	inst_amoxor_d->jit_func	 = &jit::jit_AMOXOR_D;
	inst_amoand_d->jit_func	 = &jit::jit_AMOAND_D;
	inst_amoor_d->jit_func	 = &jit::jit_AMOOR_D;
	inst_amomin_d->jit_func	 = &jit::jit_AMOMIN_D;
	inst_amomax_d->jit_func	 = &jit::jit_AMOMAX_D;
	inst_amominu_d->jit_func = &jit::jit_AMOMINU_D;
	inst_amomaxu_d->jit_func = &jit::jit_AMOMAXU_D;
#endif
}
