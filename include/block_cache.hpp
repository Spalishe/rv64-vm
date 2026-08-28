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
#include "decode.hpp"
#include "self_mod.hpp"
#include <cstdint>

namespace rv64vm::runner
{
	constexpr uint32_t BLOCK_MAX_INSTS = 16;

	struct BlockInstr
	{
		const Instruction* inst; // decoded handler
		InstructionData data;	 // predecoded operands
		uint8_t increase_pc;
	};

	struct Block
	{
		uint64_t start_phys;
		uint64_t gen;		// block-cache generation at compile time
		uint64_t smc;		// self-mod epoch at compile time
		uint32_t count;
		uint32_t last_inst; // FENCE.I detection
		BlockInstr instrs[BLOCK_MAX_INSTS];
	};

	struct BlockCache
	{
		static constexpr uint32_t CACHE_BITS = 14;
		static constexpr uint32_t CACHE_SIZE = 1u << CACHE_BITS;

		uint64_t generation = 1;
		Block slots[CACHE_SIZE];

		void clear() { generation++; }

		Block* lookup(uint64_t phys_pc)
		{
			Block& b = slots[(phys_pc >> 2) & (CACHE_SIZE - 1)];
			if(b.start_phys == phys_pc && b.gen == generation && b.smc == g_smc_epoch.load()) [[likely]]
				return &b;
			return nullptr;
		}
	};
}