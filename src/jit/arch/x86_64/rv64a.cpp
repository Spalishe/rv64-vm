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

	static inline bool atomic_op(Hart&, InstructionData& d, JIT_Block&, JIT_Emitter& em, AMOOp op, uint8_t width)
	{
		em.emit_atomic(d.rd, d.rs1, d.rs2, op, width);
		return !em.eof();
	}

	bool jit_LR_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::LR, 4);
	}
	bool jit_SC_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::SC, 4);
	}
	bool jit_AMOSWAP_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::SWAP, 4);
	}
	bool jit_AMOADD_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::ADD, 4);
	}
	bool jit_AMOXOR_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::XOR, 4);
	}
	bool jit_AMOAND_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::AND, 4);
	}
	bool jit_AMOOR_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::OR, 4);
	}
	bool jit_AMOMIN_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MIN, 4);
	}
	bool jit_AMOMAX_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MAX, 4);
	}
	bool jit_AMOMINU_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MINU, 4);
	}
	bool jit_AMOMAXU_W(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MAXU, 4);
	}

	bool jit_LR_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::LR, 8);
	}
	bool jit_SC_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::SC, 8);
	}
	bool jit_AMOSWAP_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::SWAP, 8);
	}
	bool jit_AMOADD_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::ADD, 8);
	}
	bool jit_AMOXOR_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::XOR, 8);
	}
	bool jit_AMOAND_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::AND, 8);
	}
	bool jit_AMOOR_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::OR, 8);
	}
	bool jit_AMOMIN_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MIN, 8);
	}
	bool jit_AMOMAX_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MAX, 8);
	}
	bool jit_AMOMINU_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MINU, 8);
	}
	bool jit_AMOMAXU_D(Hart& h, InstructionData& d, JIT_Block& b, JIT_Emitter& e)
	{
		return atomic_op(h, d, b, e, AMOOp::MAXU, 8);
	}
}

#endif
