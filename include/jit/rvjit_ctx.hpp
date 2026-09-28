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
#include <cstddef>
#include <cstdint>

#include "../tlb.hpp"

namespace rv64vm::runner
{
	class MMIO;
	class Hart;
}

/*
 * Guest state layout shared with the generated code, which addresses these
 * fields by fixed offsets — keep the field order.
 */
namespace rv64vm::jit
{
	struct JIT_HartContext;
	using JITCompiledFunc = void (*)(JIT_HartContext*);

	// Direct-mapped jump cache: the chain dispatcher probes it on every private
	// slot miss to find the target block's post-prologue entry.
	//
	// Sizing note: this table is the only thing that lets a chain hop into a
	// successor *without* a C++ round trip, and it is filled exclusively by the
	// C++ dispatch loop (one install per block it enters).  Kernel boot
	// compiles ~90k distinct block starts, so a table smaller than that
	// overflows chronically and every overflow turns into a chain break: the
	// target's slot is either still zeroed (dispatcher's "nofn") or holds a
	// different block at the same index ("pc").  Keep the table comfortably
	// above the live block count.
#ifndef USE_JCHAIN
	#define USE_JCHAIN 18
#endif
	constexpr uint32_t CHAIN_CACHE_SLOTS = 1u << USE_JCHAIN;
	constexpr uint32_t CHAIN_CACHE_MASK  = CHAIN_CACHE_SLOTS - 1;

	// Index of `pc` in the chain cache: the top CHAIN_SLOTS_LOG2 bits of the
	// 64-bit golden-ratio product of (pc>>1).  Guest pcs are 2-byte aligned
	// (and 4-byte aligned for any uncompressed instruction), so pc>>1 is odd
	// or even uniformly and the multiply mixes the low address bits up into
	// the high ones.
	//
	// The privilege mode is mixed in so that the same pc compiled for
	// different modes lands in different slots and they can coexist.  A mode
	// change used to bump chain_epoch, which invalidated *every* entry at
	// once; on a syscall-heavy workload that is 97% of all invalidations, and
	// it left the chain cache permanently cold (measured 9% hit rate, ~11
	// guest instructions per C++ entry instead of the 12288 the budget
	// allows).  Blocks still bake their own mode into their TLB checks, so
	// keeping modes apart here is a pure cache-locality fix, not a change of
	// the validation rules: the mode is still checked, per entry.
	//
	// The C++ dispatch loop and the generated dispatcher must agree on this
	// exactly; the generated form is in JIT_Context::ensure_chain_dispatcher().
	inline uint32_t chain_cache_index(uint64_t pc, uint64_t mode_key)
	{
		const uint64_t a = ((pc >> 1) * 0x9E3779B97F4A7C15ULL) >> (64 - USE_JCHAIN);
		const uint64_t b = (mode_key * 0xD1B54A32D192ED03ULL) >> (64 - USE_JCHAIN);
		return (uint32_t)((a ^ b) & CHAIN_CACHE_MASK);
	}

	struct JIT_HartContext
	{
		uint64_t* regs;		// &hart.GPR[0]
		uint8_t* ram;		// host pointer to guest 0x80000000
		runner::MMIO* mmio;
		uint64_t memsize;
		uint64_t entry_pc; // set by the runner before the call
		uint64_t exit_pc;	// written by the block epilogue
		runner::Hart* hart;
		int32_t loop_count;
		int32_t reserved;
		runner::TLB::TlbEntry* tlb_entries; // refreshed by the runner per dispatch
		uint64_t tlb_gen;					// TLB::current_generation() at dispatch time
		uint16_t satp_asid;
		uint16_t pad;
		uint64_t exit_count; // instructions executed, written by the block exits
		/*
		 * Folds (tlb_gen, smc_key, satp_asid) into one stamp.  The runner bumps
		 * it whenever any of those three changes, so a link slot or chain-cache
		 * entry stamped with the current epoch is exactly as valid as
		 * revalidating all three - which is what the generated block exits used
		 * to do, at three instructions each, on every single block exit.
		 *
		 * mode_key is deliberately NOT folded in: it is checked per entry
		 * instead (see chain_cache_index and the mode word at slot offset 24).
		 * Folding it here meant one syscall wiped the whole chain cache, which
		 * is what left chains covering ~11 instructions instead of 12288.
		 */
		uint64_t chain_epoch;
		int64_t chain_budget; // instructions until the chain must return to C++
		uint64_t chain_mode;  // mode_key of the blocks valid in this chain
		// Keep the table cache-line aligned and at a stable offset: it is
		// probed on every chain hop.
		uint64_t chain_cache_pad;
		uint64_t chain_cache[CHAIN_CACHE_SLOTS * 4]; // CHAIN_CACHE_STRIDE-sized slots
	};

	static_assert(offsetof(JIT_HartContext, regs) == 0);
	static_assert(offsetof(JIT_HartContext, ram) == 8);
	static_assert(offsetof(JIT_HartContext, mmio) == 16);
	static_assert(offsetof(JIT_HartContext, memsize) == 24);
	static_assert(offsetof(JIT_HartContext, entry_pc) == 32);
	static_assert(offsetof(JIT_HartContext, exit_pc) == 40);
	static_assert(offsetof(JIT_HartContext, hart) == 48);
	static_assert(offsetof(JIT_HartContext, loop_count) == 56);
	static_assert(offsetof(JIT_HartContext, tlb_entries) == 64);
	static_assert(offsetof(JIT_HartContext, tlb_gen) == 72);
	static_assert(offsetof(JIT_HartContext, satp_asid) == 80);
	static_assert(offsetof(JIT_HartContext, exit_count) == 88);
	static_assert(offsetof(JIT_HartContext, chain_epoch) == 96);
	static_assert(offsetof(JIT_HartContext, chain_budget) == 104);
	static_assert(offsetof(JIT_HartContext, chain_mode) == 112);
	static_assert(offsetof(JIT_HartContext, chain_cache) == 128);
	static_assert(offsetof(JIT_HartContext, chain_cache) % 64 == 0);

	// Layout mirrors of TLB::TlbEntry, sanity-checked against offsetof above.
	// Do not change the TlbEntry field order without updating these.
	static_assert(offsetof(runner::TLB::TlbEntry, vpage_mask_inv) == 0);
	static_assert(offsetof(runner::TLB::TlbEntry, vpage_base) == 8);
	static_assert(offsetof(runner::TLB::TlbEntry, generation) == 16);
	static_assert(offsetof(runner::TLB::TlbEntry, host_ptr) == 24);
	static_assert(offsetof(runner::TLB::TlbEntry, ppage_base) == 32);
	static_assert(offsetof(runner::TLB::TlbEntry, asid) == 40);
	static_assert(offsetof(runner::TLB::TlbEntry, page_bits) == 42);
	static_assert(offsetof(runner::TLB::TlbEntry, perm) == 43);
	static_assert(offsetof(runner::TLB::TlbEntry, global) == 44);
	static_assert(sizeof(runner::TLB::TlbEntry) == 64);
}
