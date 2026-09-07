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

#include "rvjit_ctx.hpp"
#include "rvjit_emit.hpp"
#include "rvjit_fwd.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace rv64vm::runner
{
	class Hart;
	struct InstructionData;
}

namespace rv64vm::jit
{
	using JITCompiledFunc = void (*)(JIT_HartContext*);

	inline constexpr size_t JIT_ARENA_BYTES = RVJIT_ARENA_PAGES * 4096;

	// Result of a JIT dispatch attempt.
	struct JITExec
	{
		JITCompiledFunc fn = nullptr; // nullptr => interpreter fallback
		uint32_t count	   = 0;
	};

	class JIT_Context
	{
	  public:
		static constexpr uint64_t JIT_CACHE_SIZE = 1 << 18;

		JIT_Context() : cache(JIT_CACHE_SIZE), giveup(JIT_CACHE_SIZE) {}
		~JIT_Context() { release_arenas(); }

		JIT_Context(const JIT_Context&)			   = delete;
		JIT_Context& operator=(const JIT_Context&) = delete;

		// Compiles the current guest pc and returns a block ready for
		// execution; {nullptr,0} when the stream is not JIT-able.
		JITExec compile(runner::Hart& h, uint64_t va_pc, uint64_t phys_pc);

		// Cache lookup with ASID + SMC-epoch validation.
		JITExec lookup(uint64_t phys_pc, uint64_t asid);

		// Hotness gate: triggers compilation after RVJIT_HOT_THRESHOLD dispatches.
		bool hot_tick(uint64_t phys_pc);

		// Flush the cache and drop all compiled code.
		void invalidate_all();

	  private:
		struct CachedBlock
		{
			JITCompiledFunc fn		  = nullptr;
			uint64_t start_phys		  = 0;
			uint64_t asid			  = 0;
			uint64_t smc_epoch		  = 0;
			std::atomic<uint32_t> hot = 0; // dispatch counter before compiling
			uint32_t count			  = 0;
			bool valid				  = false;
		};

		// Per-slot "compiling this pc is pointless" marker, kept separate
		// from CachedBlock so a give-up never clobbers a live block.
		struct GiveUpSlot
		{
			uint64_t phys = 0;
			bool skip	  = false;
		};

		std::vector<CachedBlock> cache;
		std::vector<GiveUpSlot> giveup;
		std::vector<uint8_t*> arenas;
		uint8_t* cur	= nullptr;
		size_t cur_used = 0;
		std::mutex mtx;

		uint8_t* arena_alloc(size_t nbytes);
		void mark_block_executed(uint64_t phys_pc, uint64_t guest_bytes);
		void release_arenas();
		static uint64_t index_of(uint64_t phys_pc) { return (phys_pc >> 2) & (JIT_CACHE_SIZE - 1); }
	};

// Each returns true when the instruction was compiled and the block may
// continue, false when the block should stop right after it.
#define RVJIT_ISA_DECL(name) \
	bool name(runner::Hart&, runner::InstructionData&, JIT_Block&, JIT_Emitter&)

	RVJIT_ISA_DECL(jit_ADD);
	RVJIT_ISA_DECL(jit_ADDW);
	RVJIT_ISA_DECL(jit_SUB);
	RVJIT_ISA_DECL(jit_SUBW);
	RVJIT_ISA_DECL(jit_XOR);
	RVJIT_ISA_DECL(jit_OR);
	RVJIT_ISA_DECL(jit_AND);
	RVJIT_ISA_DECL(jit_SLL);
	RVJIT_ISA_DECL(jit_SLLW);
	RVJIT_ISA_DECL(jit_SRL);
	RVJIT_ISA_DECL(jit_SRLW);
	RVJIT_ISA_DECL(jit_SRA);
	RVJIT_ISA_DECL(jit_SRAW);
	RVJIT_ISA_DECL(jit_SLT);
	RVJIT_ISA_DECL(jit_SLTU);
	RVJIT_ISA_DECL(jit_ADDI);
	RVJIT_ISA_DECL(jit_ADDIW);
	RVJIT_ISA_DECL(jit_XORI);
	RVJIT_ISA_DECL(jit_ORI);
	RVJIT_ISA_DECL(jit_ANDI);
	RVJIT_ISA_DECL(jit_SLLI);
	RVJIT_ISA_DECL(jit_SLLIW);
	RVJIT_ISA_DECL(jit_SRLI);
	RVJIT_ISA_DECL(jit_SRLIW);
	RVJIT_ISA_DECL(jit_SRAI);
	RVJIT_ISA_DECL(jit_SRAIW);
	RVJIT_ISA_DECL(jit_SLTI);
	RVJIT_ISA_DECL(jit_SLTIU);
	RVJIT_ISA_DECL(jit_MUL);
	RVJIT_ISA_DECL(jit_MULW);
	RVJIT_ISA_DECL(jit_MULH);
	RVJIT_ISA_DECL(jit_MULHU);
	RVJIT_ISA_DECL(jit_MULHSU);
	RVJIT_ISA_DECL(jit_DIV);
	RVJIT_ISA_DECL(jit_DIVU);
	RVJIT_ISA_DECL(jit_DIVW);
	RVJIT_ISA_DECL(jit_DIVUW);
	RVJIT_ISA_DECL(jit_REM);
	RVJIT_ISA_DECL(jit_REMU);
	RVJIT_ISA_DECL(jit_REMW);
	RVJIT_ISA_DECL(jit_REMUW);
#undef RVJIT_ISA_DECL
}
