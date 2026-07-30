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

MemoryReturn AMO_SC(Hart& hart, uint64_t va, MemorySize size, uint64_t val, void* out_val)
{
	if(hart.get_reservation().valid && hart.get_reservation().vaddr == va && hart.get_reservation().size == size)
	{
		MemoryReturn out = hart.get_mmio()->write(hart, va, size, val);
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
	uint64_t value;
	MemoryReturn p = hart.get_mmio()->read(hart, va, size, &value);
	if(!p.is_success) return p;
	hart.get_reservation().valid = true;
	hart.get_reservation().size	 = size;
	hart.get_reservation().vaddr = va;
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
	uint64_t val;
	MemoryReturn out = AMO_LR(hart, hart.GPR[inst.rs1], MemorySize::Long, &val);
	if(!out.is_success) return { false, false, 4, out.exc_code, out.tval };
	hart.GPR[inst.rd] = val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SC_D(Hart& hart, InstructionData& inst)
{
	uint64_t val;
	MemoryReturn out = AMO_SC(hart, hart.GPR[inst.rs1], MemorySize::Long, hart.GPR[inst.rs2], &val);
	if(!out.is_success) return { false, false, 4, out.exc_code, out.tval };
	hart.GPR[inst.rd] = val;
	return { true, false, 4, 0, 0 };
}

MemoryReturn AMO64(Hart& hart, uint64_t va, uint64_t rs2, uint64_t (*func)(uint64_t a, uint64_t b), uint64_t* out_val)
{
	uint64_t val;
	MemoryReturn out = hart.get_mmio()->read(hart, va, MemorySize::Long, &val);
	if(!out.is_success) return out;
	uint64_t new_val = func(val, rs2);

	MemoryReturn s = hart.get_mmio()->write(hart, va, MemorySize::Long, new_val);
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
	uint32_t val;
	MemoryReturn out = AMO_LR(hart, hart.GPR[inst.rs1], MemorySize::Int, &val);
	if(!out.is_success) return { false, false, 0, out.exc_code, out.tval };
	hart.GPR[inst.rd] = (int64_t)(int32_t)val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SC_W(Hart& hart, InstructionData& inst)
{
	uint32_t val;
	MemoryReturn out = AMO_SC(hart, hart.GPR[inst.rs1], MemorySize::Int, (uint32_t)hart.GPR[inst.rs2], &val);
	if(!out.is_success) return { false, false, 0, out.exc_code, out.tval };
	hart.GPR[inst.rd] = val;
	return { true, false, 4, 0, 0 };
}

MemoryReturn AMO32(Hart& hart, uint64_t va, uint32_t rs2, uint32_t (*func)(uint32_t a, uint32_t b), uint32_t* out_val)
{
	uint32_t val;
	MemoryReturn out = hart.get_mmio()->read(hart, va, MemorySize::Int, &val);
	if(!out.is_success) return out;
	uint64_t new_val = func(val, rs2);

	MemoryReturn s = hart.get_mmio()->write(hart, va, MemorySize::Int, new_val);
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
	register_instr("00001************010*****0101111", exec_AMOSWAP_W);
	register_instr("00000************010*****0101111", exec_AMOADD_W);
	register_instr("01100************010*****0101111", exec_AMOAND_W);
	register_instr("00100************010*****0101111", exec_AMOXOR_W);
	register_instr("01000************010*****0101111", exec_AMOOR_W);
	register_instr("10000************010*****0101111", exec_AMOMIN_W);
	register_instr("11000************010*****0101111", exec_AMOMINU_W);
	register_instr("10100************010*****0101111", exec_AMOMAX_W);
	register_instr("11100************010*****0101111", exec_AMOMAXU_W);
	register_instr("00010************010*****0101111", exec_LR_W);
	register_instr("00011************010*****0101111", exec_SC_W);

	register_instr("00001************011*****0101111", exec_AMOSWAP_D);
	register_instr("00000************011*****0101111", exec_AMOADD_D);
	register_instr("01100************011*****0101111", exec_AMOAND_D);
	register_instr("00100************011*****0101111", exec_AMOXOR_D);
	register_instr("01000************011*****0101111", exec_AMOOR_D);
	register_instr("10000************011*****0101111", exec_AMOMIN_D);
	register_instr("11000************011*****0101111", exec_AMOMINU_D);
	register_instr("10100************011*****0101111", exec_AMOMAX_D);
	register_instr("11100************011*****0101111", exec_AMOMAXU_D);
	register_instr("00010************011*****0101111", exec_LR_D);
	register_instr("00011************011*****0101111", exec_SC_D);
}
