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

/*
 * Forward declarations so decode.hpp can hold the jit_func member without
 * pulling in the whole JIT implementation.
 */
namespace rv64vm::jit
{
	struct JIT_Block;
	struct JIT_Emitter;
}

namespace rv64vm::runner
{
	class Hart;
	struct InstructionData;

	using JITFunc = bool (*)(Hart&, InstructionData&, jit::JIT_Block&, jit::JIT_Emitter&);
}
