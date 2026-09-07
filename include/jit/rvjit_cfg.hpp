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

namespace rv64vm::jit
{
	inline constexpr size_t RVJIT_MIN_INSTRUCTIONS = 1;
	inline constexpr size_t RVJIT_MAX_INSTRUCTIONS = 128;
	inline constexpr size_t RVJIT_FUNC_SIZE		   = 0x1000; // host code bytes per block
	inline constexpr size_t RVJIT_ARENA_PAGES	   = 0x400;	 // host pages per code arena
	inline constexpr size_t RVJIT_MAX_CACHE_BYTES  = 64 * 1024 * 1024;
	// encode() may briefly overproduce bytes; this much margin keeps us off the buffer edge.
	inline constexpr size_t RVJIT_FUNC_MARGIN = 256;

	// Compile a block only after this many dispatches, so codegen cost is
	// paid only for code that actually gets reused.
	inline constexpr size_t RVJIT_HOT_THRESHOLD = 2;
}
