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
	};

	static_assert(offsetof(JIT_HartContext, regs) == 0);
	static_assert(offsetof(JIT_HartContext, ram) == 8);
	static_assert(offsetof(JIT_HartContext, mmio) == 16);
	static_assert(offsetof(JIT_HartContext, memsize) == 24);
	static_assert(offsetof(JIT_HartContext, entry_pc) == 32);
	static_assert(offsetof(JIT_HartContext, exit_pc) == 40);
	static_assert(offsetof(JIT_HartContext, hart) == 48);
	static_assert(offsetof(JIT_HartContext, loop_count) == 56);
}
