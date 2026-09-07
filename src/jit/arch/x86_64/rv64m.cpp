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
 * RV64M register-form translators (semantics mirror src/sets/rv64m.cpp;
 * x0 writes are dropped; W variants wrap to 32 bits and sign-extend).
 */
#include "../../../../include/decode.hpp"
#include "../../../../include/jit/rvjit.hpp"

#ifdef USE_JIT

namespace rv64vm::jit
{
	using namespace rv64vm::runner;

	static inline bool m_op(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, MOp op, bool w)
	{
		if(d.rd == 0)
			return !em.eof();
		em.emit_m_r_to(d.rd, d.rs1, d.rs2, op, w);
		return !em.eof();
	}

	bool jit_MUL(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::MUL, false);
	}
	bool jit_MULW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::MUL, true);
	}
	bool jit_MULH(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::MULH, false);
	}
	bool jit_MULHU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::MULHU, false);
	}
	bool jit_MULHSU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::MULHSU, false);
	}
	bool jit_DIV(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::DIV, false);
	}
	bool jit_DIVU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::DIVU, false);
	}
	bool jit_DIVW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::DIV, true);
	}
	bool jit_DIVUW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::DIVU, true);
	}
	bool jit_REM(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::REM, false);
	}
	bool jit_REMU(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::REMU, false);
	}
	bool jit_REMW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::REM, true);
	}
	bool jit_REMUW(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return m_op(h, d, b, e, MOp::REMU, true);
	}
}

#endif