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
#ifdef USE_JIT
#include "../../include/jit/rvjit.hpp"
#endif

using namespace rv64vm::runner;
ExecReturn exec_FENCE_I(Hart& hart, InstructionData& inst)
{
	hart.clear_decode_cache();
	// FENCE.I is a no-op for the JIT: instruction fetch is never cached —
	// compiled blocks validate the SMC epoch at dispatch and are recompiled
	// from RAM, and any store to a page hosting current-epoch code already
	// bumps g_smc_epoch through the W^X-stripped TLB paths (see self_mod.hpp:
	// mark_page_code re-arms the detector on every compile). So the next
	// dispatch to a rewritten page observes the patched bytes.
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zifencei()
{
	register_instr("*****************001*****0001111", exec_FENCE_I);
}
