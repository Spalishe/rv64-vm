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

#include "../../include/hart.hpp"
#include "../../include/jit/rvjit_ctx.hpp"
#include "../../include/jit/rvjit_x86_64.hpp"

namespace
{
	// Low byte count encoded in the packed descriptor (1/2/4/8).
	inline int64_t sign_extend(uint64_t raw, uint8_t width)
	{
		switch(width)
		{
			case 1: return (int64_t)(int8_t)raw;
			case 2: return (int64_t)(int16_t)raw;
			case 4: return (int64_t)(int32_t)raw;
			default: return (int64_t)raw;
		}
	}
}

extern "C" uint64_t rvjit_mem_exec(rv64vm::jit::JIT_HartContext* ctx, uint64_t packed)
{
	using namespace rv64vm;
	using namespace rv64vm::runner;

	// Layout mirrors pack_mem() in src/jit/arch/x86_64/emit.cpp.
	const uint8_t  width	   = (uint8_t)(packed & 0xF);
	const bool	   store	   = ((packed >> 4) & 1) != 0;
	const bool	   sign	   = ((packed >> 5) & 1) != 0;
	const uint8_t  rd		   = (uint8_t)((packed >> 6) & 0x1F);
	const uint8_t  rs1		   = (uint8_t)((packed >> 11) & 0x1F);
	const uint8_t  rs2		   = (uint8_t)((packed >> 16) & 0x1F);
	const uint16_t imm12	   = (uint16_t)((packed >> 21) & 0xFFF);
	const int64_t  imm		   = (int64_t)(int16_t)(imm12 << 4) >> 4;
	const uint32_t instr_index = (uint32_t)((packed >> 33) & 0xFFF);
	const uint32_t instr_bytes = (uint32_t)((packed >> 45) & 0xFFFF);

	Hart& hart = *ctx->hart;
	hart.GPR[0] = 0;
	const uint64_t addr = hart.GPR[rs1] + imm;
	const MemorySize size = (MemorySize)width;

	// The runner credits cycle/instret only when a chain returns to C++.
	// Compensate while the access runs, then undo: this block is still fully
	// credited by the runner on return.
	const uint64_t done = (uint64_t)jit::x86::CHAIN_CADENCE - (uint64_t)ctx->chain_budget;
	hart.cycle += done;
	hart.instret += done;

	// Generated code does not keep Hart::pc in sync per instruction; a fault
	// needs the precise failing pc (and writes mepc/sepc from it).
	hart.pc = ctx->entry_pc + instr_bytes;
	const uint64_t pc_before = hart.pc;

	MemoryReturn result;
	if(store)
	{
		result = hart.get_mmio()->write(hart, addr, size, hart.GPR[rs2]);
	}
	else
	{
		uint64_t raw = 0;
		result = hart.get_mmio()->read(hart, addr, size, &raw);
		if(result.is_success && rd != 0)
		{
			hart.GPR[rd] = sign ? (uint64_t)sign_extend(raw, width)
								: raw & ((width == 8) ? ~0ULL : ((1ULL << (width * 8)) - 1ULL));
		}
	}

	hart.cycle -= done;
	hart.instret -= done;
	hart.GPR[0] = 0;

	if(!result.is_success) [[unlikely]]
	{
		hart.trap(result.exc_code, result.tval, false);
		ctx->exit_pc = hart.pc;
		ctx->chain_budget -= (int64_t)instr_index;
		return 1;
	}

	// A device handler is allowed to redirect the hart (e.g. reset). Bail out
	// instead of executing more compiled code for the old state.
	if(hart.pc != pc_before) [[unlikely]]
	{
		ctx->exit_pc = hart.pc + 4;
		ctx->chain_budget -= (int64_t)instr_index + 1;
		return 1;
	}

	// The access may have installed a TLB entry or changed device state; the
	// remainder of this block keeps running with the freshly observed state.
	return 0;
}
