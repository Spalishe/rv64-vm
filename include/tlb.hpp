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
#include "defines/traps.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace rv64vm::runner
{
	/**
	 * @ingroup RV64VM-API
	 * @brief RISC-V Translation Lookaside Buffer
	 */
	class TLB
	{
	  public:
		/**
		 * @brief TLB Constructor
		 */
		TLB() {}
		/**
		 * @brief TLB Destructor
		 */
		~TLB() {}

		struct TlbEntry
		{
			uint64_t vpage_base = ~0ULL; // (va & ~page_mask)
			uint64_t ppage_base = 0;
			uint16_t asid		= 0;
			uint8_t page_bits	= 12; // 12 / 21 / 30 - log2(page size)
			uint8_t perm		= 0;  // bits R W X U A D
			bool global			= false;
			uint64_t generation = 0;
		};
		enum class TLBPermissions : uint8_t
		{
			PERM_R = 1u << 0,
			PERM_W = 1u << 1,
			PERM_X = 1u << 2,
			PERM_U = 1u << 3,
			PERM_A = 1u << 4,
			PERM_D = 1u << 5,
		};
		static constexpr size_t SIZE = 1024;

		bool lookup(uint64_t va, AccessType type, uint16_t asid, int mode, bool mxr, bool sum, uint64_t* pa);

		void insert(uint64_t va, uint64_t pa, uint8_t page_bits, uint8_t perm, uint16_t asid, bool global);

		void flush_all() { ++generation; }

		void flush_addr(uint64_t) { flush_all(); }
		void flush_asid(uint16_t) { flush_all(); }
		void flush_addr_asid(uint64_t, uint16_t) { flush_all(); }

	  private:
		static size_t index(uint64_t va) { return (va >> 12) & (SIZE - 1); }
		__attribute__((always_inline)) inline bool check_perm(uint8_t perm, AccessType type, int mode, bool mxr, bool sum);

		std::array<TlbEntry, SIZE> entries{};
		uint64_t generation = 1;
	};
}
