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
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace rv64vm::runner
{
	inline std::atomic<uint64_t> g_flush_count{ 0 };

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

		/*
		 * 64-byte entry shared with the JIT: the generated code reads these
		 * fields by fixed offsets during its inline TLB lookup, so the order
		 * below is part of the on-disk ABI (see TLB_OFF_* in rvjit_x86_64.hpp
		 * and the static_asserts in rvjit_ctx.hpp).
		 */
		struct TlbEntry
		{
			uint64_t vpage_mask_inv = ~0ULL; // ~((1<<page_bits)-1): (va^vbase)&mask_inv==0 => inside page
			uint64_t vpage_base		= ~0ULL; // va & ~page_mask
			uint64_t generation		= 0;	 // matches TLB::generation while valid
			uint64_t host_ptr		= 0;	 // host page base - vpage_base; 0 = no host mapping
			uint64_t ppage_base		= 0;
			uint16_t asid			= 0;
			uint8_t page_bits		= 12; // 12 / 21 / 30 - log2(page size)
			uint8_t perm			= 0;  // bits R W X U A D
			bool global				= false;
			uint8_t pad[19]			= {};
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
		static constexpr size_t SIZE = 2 << 16;
		static constexpr size_t SIZE_MASK = (SIZE - 1);
		static constexpr size_t TLB_ENTRY_LOG2 = 6; // 64 bytes per entry

		/**
		 * @brief Looks up in cache for TLB entry
		 * @param va Virtual Address
		 * @param type Access type
		 * @param asid ASID
		 * @param mode Privilage Mode casted to integer
		 * @param mxr Hart Status MXR bit
		 * @param sum Hart Status SUM bit
		 * @param pa Physical Address pointer
		 * @see MMU
		 * @see Hart
		 * @return Is success?
		 */
		bool lookup(uint64_t va, AccessType type, uint16_t asid, int mode, bool mxr, bool sum, uint64_t* pa);

		/**
		 * @brief Inserts new TLB entry in cache
		 * @param va Virtual Address
		 * @param pa Physical address (any byte inside the target page)
		 * @param page_bits Size of PPN page bits
		 * @param perm Permissions bit set
		 * @param asid Entry ASID
		 * @param global Entry G bit
		 * @param host_page Host pointer to the start of the guest page, or nullptr
		 *                  when the page has no direct host mapping (MMIO/IO)
		 * @see MMU
		 */
		void insert(uint64_t va, uint64_t pa, uint8_t page_bits, uint8_t perm, uint16_t asid, bool global, const void* host_page = nullptr);

		// Fast paths used by the JIT runner: the generated code addresses the
		// entry array and the current generation through the hart context.
		inline TlbEntry* jit_entries() { return entries.data(); }
		inline uint64_t current_generation() const { return generation; }

		/**
		 * @brief Strips the write/dirty capability from the entry for \p va
		 * @details W^X: once a page is executed, JITed stores must miss the TLB
		 *          so stores fall back to the interpreter, which detects
		 *          self-modifying writes and invalidates compiled code.
		 */
		inline void note_exec(uint64_t va)
		{
			TlbEntry& e = entries[index(va)];
			if(e.generation == generation && (e.perm & (int)TLBPermissions::PERM_W))
			{
				e.perm &= ~((int)TLBPermissions::PERM_W | (int)TLBPermissions::PERM_D);
			}
		}

		/**
		 * @brief Flushes all TLB entries
		 */
		void flush_all()
		{
			++generation;
			g_flush_count.fetch_add(1, std::memory_order_relaxed);
		}

		/**
		 * @brief Flushes TLB entries by address (SFENCE.VMA rs1)
		 *
		 * The TLB is direct-mapped on (va >> 12); any resident entry that
		 * could serve an access to \p va lives at index(va), regardless of
		 * page size. Invalidating in place (generation = 0) instead of
		 * bumping the global generation keeps every unrelated compiled JIT
		 * block and its baked TLB checks alive; the affected slot alone
		 * fails its baked check, gets refilled by the C++ page walk, and
		 * the stale block then runs against the fresh entry.
		 */
		void flush_addr(uint64_t va)
		{
			TlbEntry& e = entries[index(va)];
			if(e.generation == generation)
				e.generation = 0;
		}
		/**
		 * @brief Flushes all TLB entries of an ASID (SFENCE.VMA x0, rs2)
		 */
		void flush_asid(uint16_t asid)
		{
			for(TlbEntry& e : entries)
				if(e.generation == generation && !e.global && e.asid == asid)
					e.generation = 0;
		}
		/**
		 * @brief Flushes a single address mapping of an ASID (SFENCE.VMA rs1, rs2)
		 */
		void flush_addr_asid(uint64_t va, uint16_t asid)
		{
			TlbEntry& e = entries[index(va)];
			if(e.generation == generation && !e.global && e.asid == asid)
				e.generation = 0;
		}

	  private:
		static inline const size_t index(uint64_t va) { return (va >> 12) & (SIZE - 1); }
		__attribute__((always_inline)) inline bool check_perm(uint8_t perm, AccessType type, int mode, bool mxr, bool sum);

		std::array<TlbEntry, SIZE> entries{};
		uint64_t generation = 1;
	};
}
