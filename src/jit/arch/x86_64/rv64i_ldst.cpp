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

/*
 * RV64I load/store translators. Each does an inline TLB lookup (emit_load /
 * emit_store) and falls back to the interpreter on any miss; semantics mirror
 * src/sets/rv64i.cpp. Returns true when the block may keep compiling past this
 * instruction, false when it must stop right after it.
 */
#include "../../../../include/decode.hpp"
#include "../../../../include/jit/rvjit.hpp"

#ifdef USE_JIT

namespace rv64vm::jit
{
	using namespace rv64vm::runner;

	static inline bool ld(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, uint8_t width, bool sign)
	{
		em.emit_load(d.rd, d.rs1, (int64_t)d.imm, width, sign);
		return !em.eof();
	}

	static inline bool st(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, uint8_t width)
	{
		em.emit_store(d.rs1, (int64_t)d.imm, d.rs2, width);
		return !em.eof();
	}

	bool jit_LB(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return ld(h, d, b, e, 1, true);
	}
	bool jit_LBU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return ld(h, d, b, e, 1, false);
	}
	bool jit_LH(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return ld(h, d, b, e, 2, true);
	}
	bool jit_LHU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return ld(h, d, b, e, 2, false);
	}
	bool jit_LW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return ld(h, d, b, e, 4, true);
	}
	bool jit_LWU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return ld(h, d, b, e, 4, false);
	}
	bool jit_LD(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return ld(h, d, b, e, 8, true);
	}

	bool jit_SB(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return st(h, d, b, e, 1);
	}
	bool jit_SH(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return st(h, d, b, e, 2);
	}
	bool jit_SW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return st(h, d, b, e, 4);
	}
	bool jit_SD(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return st(h, d, b, e, 8);
	}
}

#endif