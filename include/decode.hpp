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
#include "defines/traps.hpp"
#include <cstdint>
#include <immintrin.h>
#include <string>
#include <vector>
#ifdef USE_JIT
#include "rvjit/rvjit_emit.hpp"
#endif

namespace rv64vm::runner
{
#define FORCE_INLINE __attribute__((always_inline)) inline
	FORCE_INLINE int32_t sext(uint32_t val, int bits)
	{
		int32_t shift = 32 - bits;
		return (int32_t)(val << shift) >> shift;
	}
	FORCE_INLINE uint32_t get_bits(uint32_t inst, int hi, int lo)
	{
		return (inst >> lo) & ((1u << (hi - lo + 1)) - 1);
	}

	static FORCE_INLINE uint64_t d_rd(uint32_t inst)
	{
		return (inst >> 7) & 0x1f; // rd in bits 11..7
	}
	static FORCE_INLINE uint64_t d_rs1(uint32_t inst)
	{
		return (inst >> 15) & 0x1f; // rs1 in bits 19..15
	}
	static FORCE_INLINE uint64_t d_rs2(uint32_t inst)
	{
		return (inst >> 20) & 0x1f; // rs2 in bits 24..20
	}
	static FORCE_INLINE uint64_t d_rs3(uint32_t inst)
	{
		return (inst >> 27) & 0x1f; // rs3 in bits 31..27
	}
	static FORCE_INLINE uint64_t d_rm(uint32_t inst)
	{
		return (inst >> 12) & 0x7; // rm in bits 13..15
	}
	static FORCE_INLINE uint64_t imm_Zicsr(uint32_t inst)
	{
		return (inst >> 20);
	}
	static FORCE_INLINE uint64_t imm_I(uint32_t inst)
	{
		// imm[11:0] = inst[31:20]
		return sext(inst >> 20, 12);
	}
	static FORCE_INLINE uint64_t imm_S(uint32_t inst)
	{
		// imm[11:5] = inst[31:25], imm[4:0] = inst[11:7]
		uint32_t imm = ((inst >> 7) & 0x1f)
					   | ((inst >> 20) & 0xfe0);

		return sext(imm, 12);
	}
	static FORCE_INLINE uint64_t imm_B(uint32_t inst)
	{
		// imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
		return sext(((get_bits(inst, 11, 8) << 1) | (get_bits(inst, 30, 25) << 5) | (get_bits(inst, 7, 7) << 11) | (get_bits(inst, 31, 31) << 12)),
					13);
	}
	static FORCE_INLINE uint64_t imm_U(uint32_t inst)
	{
		// imm[31:12] = inst[31:12]
		return (int64_t)(int32_t)(inst & 0xFFFFF000u);
	}
	static FORCE_INLINE uint64_t imm_J(uint32_t inst)
	{
		// imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
		return sext((get_bits(inst, 30, 21) << 1) | (get_bits(inst, 20, 20) << 11) | (get_bits(inst, 19, 12) << 12) | (get_bits(inst, 31, 31) << 20),
					21);
	}
	static FORCE_INLINE uint64_t shamt(uint32_t inst)
	{
		// shamt(shift amount) only required for immediate shift instructions
		// shamt[4:5] = imm[5:0]
		return (uint32_t)(imm_I(inst) & 0x1f);
	}
	static FORCE_INLINE uint64_t shamt64(uint32_t inst)
	{
		// shamt(shift amount) only required for immediate shift instructions
		// shamt[4:5] = imm[5:0]
		return (uint32_t)(imm_I(inst) & 0x3f);
	}
	static FORCE_INLINE uint64_t d_c_rd(uint16_t inst)
	{
		return (inst >> 2) & 0x7;
	}
	static FORCE_INLINE uint8_t d_c_rs1(uint16_t inst)
	{
		return (inst >> 7) & 0x7;
	}
	static FORCE_INLINE uint8_t d_c_rs2(uint16_t inst)
	{
		return (inst >> 2) & 0x1f;
	}
	static FORCE_INLINE uint64_t d_c_uimm(uint32_t inst)
	{
		// return ((get_bits(inst, 6, 6) << 2) | (get_bits(inst, 5, 5) << 3) | (get_bits(inst, 11, 11) << 4) | (get_bits(inst, 12, 12) << 5) | (((inst >> 7) & 15) << 6));
		return ((inst >> 1) & 0x3c0)
			   | ((inst >> 7) & 0x30)
			   | ((inst >> 2) & 0x8)
			   | ((inst >> 4) & 0x4);
	}
	static FORCE_INLINE uint64_t d_c_uimm_cl(uint32_t inst)
	{
		return ((get_bits(inst, 6, 6) << 2) | (((inst >> 10) & 0x7) << 3) | (get_bits(inst, 5, 5) << 6));
	}
	static FORCE_INLINE uint64_t d_c_uimm_cl1(uint32_t inst)
	{
		return ((get_bits(inst, 6, 6) << 7) | (((inst >> 10) & 0x7) << 3) | (get_bits(inst, 5, 5) << 6));
	}
	static FORCE_INLINE uint64_t d_c_nzimm(uint32_t inst)
	{
		return sext(((inst >> 2) & 0x1f) | (((inst >> 12) & 0x1) << 5), 6);
	}
	static FORCE_INLINE uint64_t d_c_uimm_arith(uint32_t inst)
	{
		return ((inst >> 2) & 0x1f) | (((inst >> 12) & 0x1) << 5);
	}
	static FORCE_INLINE uint64_t d_c_nzimm_9(uint32_t inst)
	{
		return sext((((inst >> 6 & 1) << 4) | ((inst >> 2 & 1) << 5) | ((inst >> 5 & 1) << 6) | ((inst >> 3 & 1) << 7) | ((inst >> 4 & 1) << 8) | ((inst >> 12 & 1) << 9)), 10);
	}
	static FORCE_INLINE uint64_t d_c_j_imm(uint32_t inst)
	{
		return sext((((inst >> 3 & 7) << 1) | ((inst >> 11 & 1) << 4) | ((inst >> 2 & 1) << 5) | ((inst >> 7 & 1) << 6) | ((inst >> 6 & 1) << 7) | ((inst >> 9 & 1) << 8) | ((inst >> 10 & 1) << 9) | ((inst >> 8 & 1) << 10) | ((inst >> 12) & 1) << 11), 12);
	}
	static FORCE_INLINE uint64_t d_c_b_imm(uint32_t inst)
	{
		return sext((((inst >> 3 & 0x3) << 1) | ((inst >> 10 & 0x3) << 3) | ((inst >> 2 & 0x1) << 5) | ((inst >> 5 & 0x3) << 6) | ((inst >> 12 & 0x1) << 8)), 9);
	}
	static FORCE_INLINE uint64_t d_c_uimm_lwsp(uint32_t inst)
	{
		return (((inst >> 4 & 0x7) << 2) | ((inst >> 12 & 0x1) << 5) | ((inst >> 2 & 0x3) << 6));
	}
	static FORCE_INLINE uint64_t d_c_uimm_ldsp(uint32_t inst)
	{
		return (((inst >> 5 & 0x3) << 3) | ((inst >> 12 & 0x1) << 5) | ((inst >> 2 & 0x3) << 6) | ((inst >> 4 & 0x1) << 8));
	}
	static FORCE_INLINE uint64_t d_c_uimm_swsp(uint32_t inst)
	{
		return (((inst >> 9 & 0xf) << 2) | ((inst >> 7 & 0x1) << 6) | ((inst >> 8 & 0x1) << 7));
	}
	static FORCE_INLINE uint64_t d_c_uimm_sdsp(uint32_t inst)
	{
		return (((inst >> 10 & 0x7) << 3) | ((inst >> 7 & 0x1) << 6) | ((inst >> 8 & 0x1) << 7) | ((inst >> 9 & 0x1) << 8));
	}

	struct Hart;

	struct InstructionData
	{
		uint32_t inst;
		uint8_t rs1;
		uint8_t rs2;
		uint8_t rd;
		uint64_t imm;
#ifdef USE_FPU
		uint8_t rs3;
		uint8_t rm;
#endif
	};

	struct Instruction
	{
		uint32_t mask;
		uint32_t match;
		ExecReturn (*func)(Hart& h, InstructionData& data);
		uint64_t (*imm_decode_func)(uint32_t inst);
		uint8_t size = 4;
#ifdef USE_JIT
		bool (*jit_func)(Hart& h, InstructionData& data, rv64vm::jit::JIT_Block& ctx, rv64vm::jit::JIT_Emitter& emitter) = nullptr;
#endif
	};

	struct InstructionCache
	{
		uint64_t pc;
		uint32_t inst_raw = 0;
		const Instruction* inst;
		InstructionData data;
		bool valid = false;
	};

	struct CacheSet
	{
		InstructionCache ways[2];
		uint8_t victim;
	};

	static constexpr uint32_t CACHE_SIZE = 65536;
	struct InstructionDecoder
	{
		std::vector<Instruction> instructions;

		static constexpr size_t LUT_SIZE	= 1 << 22;
		const Instruction* lut[LUT_SIZE]	= { nullptr };
		static constexpr uint32_t HASH_MASK = 0xFFF0707F;

		static constexpr size_t LUT_SIZE_16	   = 1 << 16;
		const Instruction* lut16[LUT_SIZE_16]  = { nullptr };
		static constexpr uint32_t HASH_MASK_16 = 0x0000FFFF;

		CacheSet cache[CACHE_SIZE];

		InstructionCache& decode_inst_slow(uint64_t pc, uint32_t inst);
		inline InstructionCache& decode_inst(uint64_t pc, uint32_t inst)
		{
			size_t idx	  = (pc >> 2) & (CACHE_SIZE - 1);
			/*if(cache[idx].valid && cache[idx].pc == pc) [[likely]]
			{
				hit++;
				return cache[idx];
			}*/
			CacheSet& set = cache[idx];

			if(set.ways[0].valid && set.ways[0].pc == pc && set.ways[0].inst_raw == inst)
			{
				return set.ways[0];
			}

			if(set.ways[1].valid && set.ways[1].pc == pc && set.ways[1].inst_raw == inst)
			{
				return set.ways[1];
			}

			return decode_inst_slow(pc, inst);
		}

		uint32_t global_hash_mask = 0;
		void build_lut();
		Instruction* register_instr(std::string mask, ExecReturn (*func)(Hart&, InstructionData&), uint64_t (*imm_decode_func)(uint32_t inst) = NULL);
		static inline uint32_t get_lut_index(uint32_t inst)
		{
			return _pext_u32(inst, HASH_MASK);
		}
		static inline uint32_t get_lut_index16(uint32_t inst)
		{
			return _pext_u32(inst, HASH_MASK_16);
		}
		// This function will call on init, calling all sets functions to initialize
		void init_all_instrs();
		void init_rv64i();
		void init_rv64m();
		void init_rv64a();
#ifdef USE_FPU
		void init_rv64f();
		void init_rv64d();
#endif
		void init_rv64c();
		void init_priv();
		void init_zicsr();
		void init_zifencei();
		void init_zba();
		void init_zbb();
		void init_zbc();
		void init_zbs();
		void init_zicboz();
	};
}
