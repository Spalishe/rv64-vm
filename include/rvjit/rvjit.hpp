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
#ifdef USE_JIT
#include "../decode.hpp"
#include "../mmio.hpp"
#include "rvjit_cfg.hpp"
#include <cstdint>
#include <cstring>
#include <queue>
#include <sys/mman.h>
#include <unordered_map>

#include "rvjit_emit.hpp"
namespace rv64vm::jit
{
	static constexpr size_t JIT_CACHE_SIZE = (1 << 20);

	struct JIT_HartContext
	{
		uint64_t* regs;
		uint8_t* ram;
		rv64vm::runner::MMIO* mmio;
		uint64_t memsize;
		uint64_t exit_pc = 0;
		rv64vm::runner::Hart* hart;
		int32_t loop_count = 1000;
	};

	using JITCompilatedFunc = void (*)(JIT_HartContext*);

	struct JIT_Function
	{
		JITCompilatedFunc func = nullptr;
		uint64_t offset		   = 0; // offset from arena base
		uint32_t size		   = 0; // emitted code size
		uint64_t pc			   = 0;
		uint16_t inst_size	   = 0;
		bool valid			   = false;
		uint64_t page_version  = 0; // at which page version this function was created
		uint64_t arena_index   = 0;

		JIT_Function(const JIT_Function&)			 = delete;
		JIT_Function& operator=(const JIT_Function&) = delete;

		JIT_Function(JIT_Function&& other) noexcept
			: func(other.func),
			  offset(other.offset),
			  size(other.size),
			  pc(other.pc),
			  inst_size(other.inst_size),
			  valid(other.valid),
			  arena_index(other.arena_index)
		{
			other.func		  = nullptr;
			other.offset	  = 0;
			other.size		  = 0;
			other.pc		  = 0;
			other.inst_size	  = 0;
			other.valid		  = false;
			other.arena_index = 0;
		}

		JIT_Function& operator=(JIT_Function&& other) noexcept
		{
			if(this != &other)
			{
				func		= other.func;
				offset		= other.offset;
				size		= other.size;
				pc			= other.pc;
				inst_size	= other.inst_size;
				valid		= other.valid;
				arena_index = other.arena_index;

				other.func		  = nullptr;
				other.offset	  = 0;
				other.size		  = 0;
				other.pc		  = 0;
				other.inst_size	  = 0;
				other.valid		  = false;
				other.arena_index = 0;
			}
			return *this;
		}

		JIT_Function() = default;
	};

	struct JIT_Arena
	{
		JIT_Arena() {

		};
		// Destructor
		~JIT_Arena()
		{
			if(base && size > 0)
			{
				munmap(reinterpret_cast<void*>(base), size);
			}
		}

		// Disable copy, only move
		JIT_Arena(const JIT_Arena&)			   = delete;
		JIT_Arena& operator=(const JIT_Arena&) = delete;

		// Move
		JIT_Arena(JIT_Arena&& other) noexcept
			: valid(other.valid),
			  base(other.base),
			  size(other.size),
			  used_size(other.used_size),
			  _page_size(other._page_size)
		{
			other.base		 = nullptr;
			other.size		 = 0;
			other.used_size	 = 0;
			other.valid		 = false;
			other._page_size = 0;
		}
		// Move assigment
		JIT_Arena& operator=(JIT_Arena&& other) noexcept
		{
			if(this != &other)
			{
				// Clean up our own existing memory first
				if(base && size > 0) munmap(reinterpret_cast<void*>(base), size);

				// Copy data
				base	   = other.base;
				size	   = other.size;
				valid	   = other.valid;
				used_size  = other.used_size;
				_page_size = other._page_size;

				// Reset other
				other.base		= nullptr;
				other.size		= 0;
				other.valid		= false;
				other.used_size = 0;
			}
			return *this;
		}

		bool valid		   = true;
		void* base		   = nullptr;
		uint64_t size	   = 0;
		uint64_t used_size = 0;

		JIT_Function push_function(const void* code, size_t code_size, uint64_t arena_index);
		void init()
		{
			allocate();
		}

	  private:
		uint64_t _page_size = 0;
		void allocate();
	};
	inline uint64_t jit_index(uint64_t pc)
	{
		return (pc >> 2) & (JIT_CACHE_SIZE - 1);
	}
	struct HitPage
	{
		uint16_t hits[2048];
		uint64_t ignore[2048 / 64];

		inline bool is_ignore(uint64_t pc) const
		{
			uint32_t idx = (pc & 0xFFF) >> 1;
			return (ignore[idx >> 6] & (1ull << (idx & 63)));
		}
		inline void set_ignore(uint64_t pc)
		{
			uint32_t idx = (pc & 0xFFF) >> 1;
			ignore[idx >> 6] |= 1ull << (idx & 63);
		}
	};
	struct JIT_Context
	{
		JIT_Context(uint64_t memory_size) : memory_size(memory_size)
		{
			last_arena		   = 0;
			emitter			   = JIT_Emitter();
			jits			   = new JIT_Function[JIT_CACHE_SIZE];
			page_verion_bitmap = new uint64_t[memory_size >> 12]{};
			createNewArena();
		};
		~JIT_Context()
		{
			if(jits)
				delete[] jits;
			if(page_verion_bitmap)
				delete[] page_verion_bitmap;
		}

		// Forbid copy
		JIT_Context(const JIT_Context&)			   = delete;
		JIT_Context& operator=(const JIT_Context&) = delete;

		// Move constructor
		JIT_Context(JIT_Context&& other) noexcept
			: last_arena(other.last_arena), jits(std::move(other.jits)),
			  arenas(std::move(other.arenas)),
			  block_c(other.block_c), block(std::move(other.block)), arena_order(std::move(other.arena_order))
		{
			// Copy pc_hits
			// memcpy(pc_hits, other.pc_hits, sizeof(pc_hits));
			// memcpy(&ignore_pc, &other.ignore_pc, sizeof(ignore_pc));
		}
		// Move assigment
		JIT_Context& operator=(JIT_Context&& other) noexcept
		{
			if(this != &other)
			{
				jits   = std::move(other.jits);
				arenas = std::move(other.arenas);
				// memcpy(&ignore_pc, &other.ignore_pc, sizeof(ignore_pc));

				block_c		= other.block_c;
				block		= std::move(other.block);
				last_arena	= other.last_arena;
				arena_order = std::move(other.arena_order);
			}

			return *this;
		}

		std::queue<uint64_t> arena_order;
		size_t total_allocated = 0;
		size_t max_cache_size  = 64 * 1024 * 1024; // 64 MB by default

		JIT_Function* jits;
		std::unordered_map<uint64_t, JIT_Arena> arenas;
		bool block_c = false;
		JIT_Block block;

		uint64_t* page_verion_bitmap;

		uint64_t last_arena	 = 0;
		uint64_t count		 = 0;
		uint64_t memory_size = 0;

		JIT_Emitter emitter;

		void handleInstruction(::rv64vm::runner::Hart& h, ::rv64vm::runner::InstructionCache& cache, uint64_t prev_pc);

		void stopBlock();
		void createNewArena();
	};

	void init_jit_rv64i();
	inline void init_jit_all_instrs()
	{
		init_jit_rv64i();
	}
}
#endif
