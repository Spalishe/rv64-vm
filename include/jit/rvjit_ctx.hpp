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
