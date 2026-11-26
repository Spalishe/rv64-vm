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

#include "../../../../include/decode.hpp"
#include "../../../../include/jit/rvjit.hpp"

#ifdef USE_JIT

namespace rv64vm::jit
{
	using namespace rv64vm::runner;

	// Defined in src/sets/zicsr.cpp next to the interpreter exec functions, so
	// the helper can call them without duplicating any CSR semantics.
	extern "C" uint64_t rvjit_zicsr_exec(JIT_HartContext* ctx, uint64_t packed, uint32_t inst);

	// packed layout (shared with the helper):
	//   [2:0]   op code
	//   [14:3]  csr number
	//   [19:15] rs1 / uimm
	//   [24:20] rd
	//   [40:25] instruction index in this block
	//   [56:41] guest bytes executed before this instruction
	static inline bool csr_op(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& em, uint32_t op)
	{
		const uint64_t packed = (uint64_t)op
								| ((uint64_t)(d.imm & 0xFFF) << 3)
								| ((uint64_t)(d.rs1 & 0x1F) << 15)
								| ((uint64_t)(d.rd & 0x1F) << 20)
								| ((uint64_t)b.instr_index << 25)
								| ((uint64_t)b.instr_bytes << 41);
		em.emit_csr((uint64_t)&rvjit_zicsr_exec, packed, d.inst);
		return !em.eof();
	}

	bool jit_CSRRW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return csr_op(h, d, b, e, 0);
	}

	bool jit_CSRRS(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return csr_op(h, d, b, e, 1);
	}

	bool jit_CSRRC(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return csr_op(h, d, b, e, 2);
	}

	bool jit_CSRRWI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return csr_op(h, d, b, e, 3);
	}

	bool jit_CSRRSI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return csr_op(h, d, b, e, 4);
	}

	bool jit_CSRRCI(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return csr_op(h, d, b, e, 5);
	}
}

#endif
