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
#pragma once
#include <string>
#include <vector>
#ifdef USE_JIT
#include "rvjit_cfg.hpp"

#include "../host.hpp"
#ifdef HOST_TARGET_X86_64
#define HOST_REGS_COUNT 8
#endif

#include <cstdint>
namespace rv64vm
{
	namespace runner
	{
		class Hart;
		struct InstructionData;
	}
}
namespace rv64vm::jit
{
	struct HReg
	{
		uint8_t host_reg;
		bool used		  = false;
		uint8_t vreg	  = 0xFF;
		uint64_t last_use = 0;
		uint8_t idx		  = 0;
	};
	struct VReg
	{
		bool allocated = false;
		uint8_t vreg;
		uint8_t host_reg = 0xFF;
		uint8_t host_idx = 0;
		bool dirty		 = false;
		bool valid		 = false;
		bool is_zero	 = false;
	};
	struct JumpLabel
	{
		std::string label;
		uint64_t offs;
		bool is_opcode_2	   = false;
		size_t size			   = 4;
		int64_t determined_pos = INT64_MIN;
	};

	struct JIT_Block
	{
		JIT_Block()
			: bytes{}, inst_addr_jmp{}, byte_pos(0), valid(false), pc(0), size(0), count(0)
		{
		}
		JIT_Block(const JIT_Block&)			   = delete;
		JIT_Block& operator=(const JIT_Block&) = delete;

		JIT_Block(JIT_Block&& other) noexcept
			: byte_pos(other.byte_pos), valid(other.valid), pc(other.pc),
			  size(other.size), count(other.count),
			  jmp_labels(std::move(other.jmp_labels))
		{
			std::copy(std::begin(other.bytes), std::end(other.bytes), std::begin(bytes));
			std::copy(std::begin(other.inst_addr_jmp), std::end(other.inst_addr_jmp), std::begin(inst_addr_jmp));
		}

		JIT_Block& operator=(JIT_Block&& other) noexcept
		{
			if(this != &other)
			{
				std::copy(std::begin(other.bytes), std::end(other.bytes), std::begin(bytes));
				std::copy(std::begin(other.inst_addr_jmp), std::end(other.inst_addr_jmp), std::begin(inst_addr_jmp));
				byte_pos   = other.byte_pos;
				valid	   = other.valid;
				pc		   = other.pc;
				size	   = other.size;
				count	   = other.count;
				jmp_labels = std::move(other.jmp_labels);
			}
			return *this;
		}
		uint8_t bytes[RVJIT_FUNC_SIZE];
		uint16_t byte_pos = 0;
		std::vector<JumpLabel> jmp_labels;
		uint64_t inst_addr_jmp[RVJIT_FUNC_SIZE * 4];

		uint64_t pc;
		uint64_t size  = 0;
		uint64_t count = 0;
		bool valid	   = false;
	};

	struct JIT_Emitter;

	using ROpFunction = void (*)(JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp);
	using IOpFunction = void (*)(JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp);
	using SOpFunction = void (*)(JIT_Emitter& em, JIT_Block& blk, VReg& rs1, VReg& rs2, uint64_t imm, uint64_t pc, void* tmp);
	using BOpFunction = void (*)(JIT_Emitter& em, JIT_Block& blk, VReg& rs1, VReg& rs2, uint64_t imm, uint64_t pc, void* tmp);
	using JOpFunction = void (*)(JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp);
	using UOpFunction = void (*)(JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp);

	struct JIT_Emitter
	{
		VReg vregs[32];
		HReg host_regs[HOST_REGS_COUNT];
		uint64_t global_use_counter;

		void reset();
		void rvjit_emit_prologue(JIT_Block& blk);
		void rvjit_emit_epilogue(JIT_Block& blk);
		VReg& rvjit_alloc_reg(JIT_Block& blk, uint8_t user_reg, uint64_t locked);
		void ensure_loaded(JIT_Block& blk, VReg& vreg);
		HReg* spill(JIT_Block& blk, uint64_t locked);
		void flush_all(JIT_Block& blk);
		void invalidate_all();
		void realize_label(JIT_Block& blk, const std::string& label);

		void inst_emit_r_type(::rv64vm::runner::Hart& h, ::rv64vm::runner::InstructionData& inst, JIT_Block& blk, bool optimize_if_rsz, ROpFunction emit_op, uint64_t pc = 0, void* tmp = nullptr);
		void inst_emit_i_type(::rv64vm::runner::Hart& h, ::rv64vm::runner::InstructionData& inst, JIT_Block& blk, bool optimize_if_rsz, IOpFunction emit_op, uint64_t pc = 0, void* tmp = nullptr);
		void inst_emit_s_type(::rv64vm::runner::Hart& h, ::rv64vm::runner::InstructionData& inst, JIT_Block& blk, SOpFunction emit_op, uint64_t pc = 0, void* tmp = nullptr);
		void inst_emit_b_type(::rv64vm::runner::Hart& h, ::rv64vm::runner::InstructionData& inst, JIT_Block& blk, BOpFunction emit_op, uint64_t pc = 0, void* tmp = nullptr);
		void inst_emit_j_type(::rv64vm::runner::Hart& h, ::rv64vm::runner::InstructionData& inst, JIT_Block& blk, JOpFunction emit_op, uint64_t pc = 0, void* tmp = nullptr);
		void inst_emit_u_type(::rv64vm::runner::Hart& h, ::rv64vm::runner::InstructionData& inst, JIT_Block& blk, UOpFunction emit_op, uint64_t pc = 0, void* tmp = nullptr);
	};
}

#endif
