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
ExecReturn exec_cbo_zero(Hart& hart, InstructionData& inst)
{
	constexpr uint64_t CBO_SIZE = 64;
	uint64_t va					= hart.GPR[inst.rs1] & ~(CBO_SIZE - 1);

	uint64_t pa		 = 0;
	MemoryReturn ret = hart.get_mmu().translate(&hart, AccessType::STORE, va, &pa);
	if(!ret.is_success)
	{
		return { false, false, 0, ret.exc_code, ret.tval };
	}

	uint64_t ram_base = 0x80000000;
	uint64_t ram_size = hart.get_memsize();

	if(pa >= ram_base && (pa + CBO_SIZE) <= (ram_base + ram_size))
	{
		memset(hart.get_mmap()->get_ram_direct()->get_data() + (pa - ram_base), 0, CBO_SIZE);
	}
	else
	{
		return { false, false, 0, EXC_STORE_ACCESS_FAULT, va };
	}

	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zicboz()
{
	register_instr("000000000100*****010000000001111", exec_cbo_zero);
}
