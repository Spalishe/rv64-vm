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
	inline constexpr size_t JIT_ARENA_BYTES = RVJIT_ARENA_PAGES * 4096;

	// Debug counters (temporary).
	inline std::atomic<uint64_t> g_compile_count{ 0 };
	inline std::atomic<uint64_t> g_inval_count{ 0 };
	inline std::atomic<uint64_t> g_compile_ns{ 0 };
	inline std::atomic<uint64_t> g_unique_blocks{ 0 };
	// lookup miss classification (asid no longer participates: "asid" misses
	// below are mode/mxr/sum mismatches on the same phys)
	inline std::atomic<uint64_t> g_miss_collide{ 0 };
	inline std::atomic<uint64_t> g_miss_smc{ 0 };
	inline std::atomic<uint64_t> g_miss_asid_mode{ 0 };
	inline std::atomic<uint64_t> g_lookup_ok{ 0 };
	inline std::atomic<uint64_t> g_miss_invalid{ 0 };
	// Bumped by release_arenas(); a compiled block records the generation its
	// code was emitted into so salvage() never resurrects freed (UAF) code.
	inline std::atomic<uint64_t> g_arena_gen{ 0 };

	// Result of a JIT dispatch attempt.
	struct JITExec
	{
		JITCompiledFunc fn		 = nullptr; // nullptr => interpreter fallback
		JITCompiledFunc chain_fn = nullptr; // fn + prologue size (chain entry)
		uint32_t count			 = 0;
	};

	class JIT_Context
	{
	  public:
		static constexpr uint64_t JIT_CACHE_SIZE = 1 << 18;
		// Associativity of the block cache.  The key is asid-free (see
		// lookup()), so the ways no longer hold per-asid variants of one
		// phys - they absorb the remaining key dimensions (eff_mode/mxr/sum)
		// and plain index collisions.  Eviction is LFU by hit count.
		static constexpr size_t CACHE_WAYS = 8;

		JIT_Context()
			: cache(JIT_CACHE_SIZE * CACHE_WAYS), giveup(JIT_CACHE_SIZE), hotmap(JIT_CACHE_SIZE)
		{
		}
		~JIT_Context() { release_arenas(); }

		JIT_Context(const JIT_Context&)			   = delete;
		JIT_Context& operator=(const JIT_Context&) = delete;

		// Compiles the current guest pc and returns a block ready for
		// execution; {nullptr,0} when the stream is not JIT-able.
		JITExec compile(runner::Hart& h, uint64_t va_pc, uint64_t phys_pc);

		// Cache lookup keyed by (start_phys, SMC-epoch, eff_mode, mxr, sum).
		//
		// ASID is intentionally NOT part of the key: the inline TLB check in
		// emit_tlb_checks validates the asid (or global bit) of every data
		// access at runtime, and two structural invariants make the baked
		// instruction stream asid-independent as well:
		//   - compile() never crosses a 4K VA page, so dispatch's single
		//     VA->PA check on the start pc covers the whole block: any asid
		//     (or VA alias) that resolves the start to the same phys resolves
		//     every byte of the block to the same phys page;
		//   - AUIPC/branches read entry_pc at runtime, so no absolute VA is
		//     baked (see emit_u_to / emit_block_exit).
		// Without this, ~100 guest processes recompiled the same phys blocks
		// once per asid and the 8 ways could not hold the variants.
		JITExec lookup(uint64_t phys_pc, uint8_t eff_mode, bool mxr, bool sum);

		// Hotness gate: triggers compilation after RVJIT_HOT_THRESHOLD dispatches.
		bool hot_tick(uint64_t phys_pc);

		// After an invalidation (smc-epoch bump) the cache used to rewrite
		// every block on its next dispatch even though sfences / arena
		// flushes rarely change guest text at all.  A straight-line compiled
		// block is fully determined by its guest bytes, so a stale-epoch way
		// whose text hashes identically to the current physical text is reused
		// and re-keyed to the current epoch instead of recompiled.  Returns
		// false when nothing can be salvaged (caller must compile).
		bool salvage(runner::Hart& h, uint64_t phys_pc, uint8_t eff_mode, bool mxr, bool sum, JITExec& out);

		// Permanent "not a JIT-able starter" decision (see compile()); lets the
		// dispatch memo cache the interpreter fallback for this phys.
		bool is_giveup(uint64_t phys_pc);

		// Flush the cache and drop all compiled code.
		void invalidate_all();

	  private:
		struct CachedBlock
		{
			JITCompiledFunc fn		 = nullptr;
			JITCompiledFunc chain_fn = nullptr; // fn + prologue size
			uint64_t start_phys		 = 0;
			uint64_t smc_epoch		 = 0;
			uint8_t eff_mode		 = 0; // baked privilege mode (0=U,1=S,3=M)
			bool mxr				 = false;
			bool sum				 = false;
			uint32_t hits			 = 0; // lookup hits (LFU eviction score)
			uint32_t count			 = 0;
			uint32_t guest_bytes	 = 0; // guest text bytes this block decoded
			uint64_t text_hash[2]	 = { 0, 0 }; // 128-bit digest of that text
			uint64_t arena_gen		 = 0; // release_arenas generation (UAF guard)
			bool valid				 = false;
		};

		// Per-slot "compiling this pc is pointless" marker, kept separate
		// from CachedBlock so a give-up never clobbers a live block.
		struct GiveUpSlot
		{
			uint64_t phys = 0;
			bool skip	  = false;
		};

		// Per-bucket hotness gate, keyed by phys only (not by asid), so the
		// "compile this pc" decision matches the single-slot semantics: a hot
		// phys compiles whichever (asid, ...) variant is currently dispatching.
		struct HotSlot
		{
			uint32_t hot	   = 0; // dispatch counter before compiling
			uint32_t hot_epoch = 0; // smc epoch the hot counter is based on
		};

		// JIT_CACHE_SIZE buckets x CACHE_WAYS ways; bucket b lives at
		// cache[b * CACHE_WAYS + w].  Each way is keyed by
		// (start_phys, smc_epoch, eff_mode, mxr, sum) - no asid, no VA.
		std::vector<CachedBlock> cache;
		std::vector<GiveUpSlot> giveup;
		std::vector<HotSlot> hotmap;
		std::vector<uint8_t*> arenas;
		uint8_t* cur		 = nullptr;
		size_t cur_used		 = 0;
		std::atomic_flag mtx = ATOMIC_FLAG_INIT;

		uint8_t* arena_alloc(size_t nbytes);
		void mark_block_executed(uint64_t phys_pc, uint64_t guest_bytes);
		void release_arenas();
		static bool text_hash_phys(runner::Hart& h, uint64_t phys, uint32_t len, uint64_t out[2]);
		static uint64_t index_of(uint64_t phys_pc)
		{
			// Mix high address bits into the index: kernel text spans tens of
			// MB, and masking only the low bits aliased every 512 KiB, evicting
			// live blocks and forcing recompiles.
			uint64_t h = phys_pc >> 1;
			h ^= h >> 17;
			h *= 0x9E3779B97F4A7C15ULL;
			return (h >> 20) & (JIT_CACHE_SIZE - 1);
		}
	};

	extern std::atomic<uint64_t> g_slv_ok;
	extern std::atomic<uint64_t> g_slv_mod;
	extern std::atomic<uint64_t> g_slv_dead;
	extern std::atomic<uint64_t> g_slv_text;
	extern std::atomic<uint64_t> g_slv_unread;
	extern std::atomic<uint64_t> g_slv_none;

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

	RVJIT_ISA_DECL(jit_LB);
	RVJIT_ISA_DECL(jit_LBU);
	RVJIT_ISA_DECL(jit_LH);
	RVJIT_ISA_DECL(jit_LHU);
	RVJIT_ISA_DECL(jit_LW);
	RVJIT_ISA_DECL(jit_LWU);
	RVJIT_ISA_DECL(jit_LD);
	RVJIT_ISA_DECL(jit_SB);
	RVJIT_ISA_DECL(jit_SH);
	RVJIT_ISA_DECL(jit_SW);
	RVJIT_ISA_DECL(jit_SD);

	RVJIT_ISA_DECL(jit_LUI);
	RVJIT_ISA_DECL(jit_AUIPC);

	// B-Type / jumps (end the block with a native exit).
	RVJIT_ISA_DECL(jit_BEQ);
	RVJIT_ISA_DECL(jit_BNE);
	RVJIT_ISA_DECL(jit_BLT);
	RVJIT_ISA_DECL(jit_BGE);
	RVJIT_ISA_DECL(jit_BLTU);
	RVJIT_ISA_DECL(jit_BGEU);
	RVJIT_ISA_DECL(jit_JAL);
	RVJIT_ISA_DECL(jit_JALR);

	// Compressed (RV64C) translators; FP C instructions stay on the
	// interpreter (no jit_func).
	RVJIT_ISA_DECL(jit_C_NOP);
	RVJIT_ISA_DECL(jit_C_ADDI4SPN);
	RVJIT_ISA_DECL(jit_C_ADDI);
	RVJIT_ISA_DECL(jit_C_ADDIW);
	RVJIT_ISA_DECL(jit_C_LI);
	RVJIT_ISA_DECL(jit_C_LUI_ADDI16SP);
	RVJIT_ISA_DECL(jit_C_SLLI);
	RVJIT_ISA_DECL(jit_C_SRLI);
	RVJIT_ISA_DECL(jit_C_SRAI);
	RVJIT_ISA_DECL(jit_C_ANDI);
	RVJIT_ISA_DECL(jit_C_SUB);
	RVJIT_ISA_DECL(jit_C_XOR);
	RVJIT_ISA_DECL(jit_C_OR);
	RVJIT_ISA_DECL(jit_C_AND);
	RVJIT_ISA_DECL(jit_C_SUBW);
	RVJIT_ISA_DECL(jit_C_ADDW);
	RVJIT_ISA_DECL(jit_C_MV);
	RVJIT_ISA_DECL(jit_C_ADD);
	RVJIT_ISA_DECL(jit_C_LW);
	RVJIT_ISA_DECL(jit_C_LD);
	RVJIT_ISA_DECL(jit_C_SW);
	RVJIT_ISA_DECL(jit_C_SD);
	RVJIT_ISA_DECL(jit_C_LWSP);
	RVJIT_ISA_DECL(jit_C_LDSP);
	RVJIT_ISA_DECL(jit_C_SWSP);
	RVJIT_ISA_DECL(jit_C_SDSP);
	RVJIT_ISA_DECL(jit_C_J);
	RVJIT_ISA_DECL(jit_C_BEQZ);
	RVJIT_ISA_DECL(jit_C_BNEZ);
	RVJIT_ISA_DECL(jit_C_JR);
	RVJIT_ISA_DECL(jit_C_JALR);
#undef RVJIT_ISA_DECL
}
