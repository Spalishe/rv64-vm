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

#include "../include/decode.hpp"
#include <assert.h>
#include <bitset>
#include <cstdio>

__attribute__((noinline)) InstructionCache& InstructionDecoder::decode_inst_slow(uint64_t pc, uint32_t inst)
{
	size_t idx = (pc >> 2) & (CACHE_SIZE - 1);

	const Instruction* dinst = nullptr;

	if((inst & 0x3) != 0x3)
	{
		uint32_t lut_idx = _pext_u32(inst & 0xFFFF, HASH_MASK_16);
		dinst			 = lut16[lut_idx];
	}

	if(!dinst)
	{
		uint32_t lut_idx = _pext_u32(inst, HASH_MASK);
		dinst			 = lut[lut_idx];
	}

	bool f = (dinst != nullptr) && ((inst & dinst->mask) == dinst->match);
	InstructionData data;
	data.inst = inst;
	data.rd	  = d_rd(inst);
	data.rs1  = d_rs1(inst);
	data.rs2  = d_rs2(inst);
#ifdef USE_FPU
	data.rs3 = d_rs3(inst);
	data.rm	 = d_rm(inst);
#endif

	CacheSet& set			= cache[idx];
	InstructionCache& entry = set.ways[set.victim];
	set.victim ^= 1;

	if(f) [[likely]]
	{
		data.imm	   = dinst->imm_decode_func(inst);
		// cache[idx] = { pc, inst, dinst, data, true };
		entry.pc	   = pc;
		entry.inst_raw = inst;
		entry.inst	   = dinst;
		entry.data	   = data;
		entry.valid	   = true;
	}
	else
	{
		data.imm = 0;
		Instruction* invalid_inst{};
		// cache[idx] = { pc, inst, invalid_inst, data, false };
		entry.pc	   = pc;
		entry.inst_raw = inst;
		entry.inst	   = invalid_inst;
		entry.data	   = data;
		entry.valid	   = false;
	}

	// printf("inst=0x%lx match=0x%lx mask=0x%lx func=%p\n", inst, dinst->match, dinst->mask, dinst->func);
	return entry;
}

Instruction* InstructionDecoder::register_instr(std::string mask, ExecReturn (*func)(Hart&, InstructionData&), uint64_t (*imm_decode_func)(uint32_t inst))
{
	assert((mask.size() == 16 || mask.size() == 32) && "Instruction mask size isn't 32 or 16 bits, good luck finding this broken instruction.");
	uint32_t inst_mask	= 0;
	uint32_t inst_match = 0;
	for(int i = 0; i < mask.size(); i++)
	{
		char c = mask[(mask.size() - 1) - i];
		if(c == '1')
		{
			inst_match |= (1u << i);
			inst_mask |= (1u << i);
		}
		else if(c == '0')
		{
			inst_mask |= (1u << i);
		}
		else if(c == '*')
		{
			// dont care
		}
	}

	Instruction inst{
		inst_mask,
		inst_match,
		func,
		(imm_decode_func == NULL) ? imm_I : imm_decode_func, (uint8_t)(mask.size() / 8) // default decode func if user dont provide such
	};
	// printf("Registered instr mask=0x%dx match=0x%dx with func=%p, imm_decode_func=%p\n", inst_mask, inst_match, func, imm_decode_func);
	instructions.push_back(inst);
	// std::string opcode_str = mask.substr(mask.length() - 7);
	// std::bitset<7> bits(opcode_str);
	// unsigned long opcode = bits.to_ulong();
	// opcode_table[opcode].push_back(inst);
	global_hash_mask |= inst_mask;
	return &instructions.back();
}

void InstructionDecoder::init_all_instrs()
{
	instructions.reserve(512);
	init_rv64i();
	init_rv64m();
	init_rv64a();
#ifdef USE_FPU
	init_rv64f();
	init_rv64d();
#endif
	init_rv64c();
	init_priv();
	init_zicsr();
	init_zifencei();
	init_zba();
	init_zbb();
	init_zbc();
	init_zbs();
	init_zicboz();
	build_lut();
}
void InstructionDecoder::build_lut()
{
	for(const auto& inst : instructions)
	{
		bool is_16bit = (inst.size == 2);

		uint32_t current_hash_mask		= is_16bit ? HASH_MASK_16 : HASH_MASK;
		size_t current_lut_size			= is_16bit ? LUT_SIZE_16 : LUT_SIZE;
		const Instruction** current_lut = is_16bit ? lut16 : lut;

		uint32_t effective_mask = inst.mask & current_hash_mask;
		uint32_t dont_care_bits = current_hash_mask & (~effective_mask);

		uint32_t sub_mask = 0;
		do
		{
			uint32_t test_inst = (inst.match & current_hash_mask) | sub_mask;
			uint32_t lut_idx   = _pext_u32(test_inst, current_hash_mask);

			if(lut_idx >= current_lut_size)
			{
				printf("[CRITICAL] Index %u out of bounds for %s LUT!\n",
					   lut_idx, is_16bit ? "16-bit" : "32-bit");
				continue;
			}

			if(current_lut[lut_idx] != nullptr)
			{
				if(__builtin_popcount(inst.mask) > __builtin_popcount(current_lut[lut_idx]->mask))
				{
					current_lut[lut_idx] = &inst;
				}
			}
			else
			{
				current_lut[lut_idx] = &inst;
			}

			sub_mask = (sub_mask - dont_care_bits) & dont_care_bits;
		} while(sub_mask != 0);
	}
}
