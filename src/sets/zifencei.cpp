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

#include "../../include/decode.hpp"
#include "../../include/hart.hpp"
#include "../../include/rvjit/rvjit.hpp"

ExecReturn exec_FENCE_I(Hart& hart, InstructionData& inst)
{
	hart.clear_decode_cache();
#ifdef USE_JIT
	// FIXED: This block is not more in use, now blocks remove themself automatically
	/*memset(hart.jctx->jits, 0, sizeof(hart.jctx->jits));
	memset(hart.jctx->ignore_pc, 0, sizeof(hart.jctx->ignore_pc));
	hart.jctx->arenas.clear();
	hart.jctx->count   = 0;
	hart.jctx->block_c = false;
	__builtin___clear_cache(nullptr, nullptr);
	hart.jctx->last_arena = 0;
	hart.jctx->createNewArena();*/
#endif
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zifencei()
{
	register_instr("*****************001*****0001111", exec_FENCE_I);
}
