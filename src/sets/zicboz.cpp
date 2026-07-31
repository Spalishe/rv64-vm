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
	uint64_t addr = hart.GPR[inst.rs1];
	// align
	addr		  = addr & ~63;
	if(addr >= 0x80000000)
	{
		// Effectively zero the memory
		memset(hart.get_mmap()->get_ram_direct()->get_data() + (addr - 0x80000000), 0, 64);
	}
	else
	{
		// Fallback for devices
		for(size_t i = 0; i < 64; ++i)
		{
			hart.get_mmio()->write(hart, addr + i, MemorySize::Byte, 0);
		}
	}
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zicboz()
{
	register_instr("000000000100*****010000000001111", exec_cbo_zero);
}
