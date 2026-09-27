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
#include <vector>

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
		 * @brief Drops W|D from the TLB entries that map \p phys
		 * @details W^X: once a page is executed, JITed stores must miss the TLB
		 *          so stores fall back to the interpreter, which detects
		 *          self-modifying writes and invalidates compiled code.
		 *
		 *          \p phys is a PHYSICAL page address - the code page the JIT has
		 *          just compiled into - not a virtual one.  The TLB is
		 *          direct-mapped by VA and one physical page can be aliased by
		 *          any number of VAs (identity vs. linear map, per-process
		 *          mm), so the entries to fix are found by physical coverage,
		 *          never by hashing \p phys as if it were a VA.
		 *
		 *          w_writable answers in O(1) whether any resident entry still
		 *          advertises W|D for this page.  Kernel text is mapped
		 *          read+execute, so that count is normally zero and this is a
		 *          no-op; only a page that really was writable data before it
		 *          started hosting code pays for an invalidation.
		 */
		inline void note_exec(uint64_t phys)
		{
			const uint64_t page = phys >> 12;
			if(!w_writable_oversized && page < w_writable.size() && w_writable[page] != 0)
			{
				// The resident entries were filled while the page was still data.
				// Drop them; every refill then goes through insert()'s physical
				// was_page_executed() rule and comes back without W|D.
				flush_all();
			}
		}

		/**
		 * @brief Flushes all TLB entries
		 */
		void flush_all()
		{
			++generation;
			std::fill(w_writable.begin(), w_writable.end(), 0);
			w_writable_oversized = false;
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
			{
				drop_writable(e);
				e.generation = 0;
			}
		}
		/**
		 * @brief Flushes all TLB entries of an ASID (SFENCE.VMA x0, rs2)
		 */
		void flush_asid(uint16_t asid)
		{
			for(TlbEntry& e : entries)
				if(e.generation == generation && !e.global && e.asid == asid)
				{
					drop_writable(e);
					e.generation = 0;
				}
		}
		/**
		 * @brief Flushes a single address mapping of an ASID (SFENCE.VMA rs1, rs2)
		 */
		void flush_addr_asid(uint64_t va, uint16_t asid)
		{
			TlbEntry& e = entries[index(va)];
			if(e.generation == generation && !e.global && e.asid == asid)
			{
				drop_writable(e);
				e.generation = 0;
			}
		}

	  private:
		static inline const size_t index(uint64_t va) { return (va >> 12) & (SIZE - 1); }
		__attribute__((always_inline)) inline bool check_perm(uint8_t perm, AccessType type, int mode, bool mxr, bool sum);

		std::array<TlbEntry, SIZE> entries{};
		uint64_t generation = 1;
		// Physical page -> how many resident entries still advertise W|D for it
		// (saturating; a page needs only a truthy count).  Lets note_exec()
		// decide in O(1) whether a page that just started hosting code has
		// anything to invalidate, instead of scanning all SIZE entries - the
		// TLB is direct-mapped by VA, so the wanted slots cannot be addressed
		// from a physical page number.
		//
		// A writable mapping wider than the cap cannot be credited page by page
		// (a 512 MiB guest page would be 128Ki credits per insert), so it sets
		// w_writable_oversized and note_exec() falls back to flushing whenever
		// it cannot prove the page is clean.  Kernel text is mapped r-x, so the
		// fallback is off in the common case.
		static constexpr uint64_t W_CREDIT_CAP_PAGES = 512; // 2 MiB, i.e. one THP
		std::vector<uint8_t> w_writable;
		bool w_writable_oversized = false;

		inline void add_writable(const TlbEntry& e)
		{
			if(!(e.perm & (int)TLBPermissions::PERM_W))
				return;
			const uint64_t first = e.ppage_base >> 12;
			const uint64_t pages = 1ULL << (e.page_bits - 12);
			if(pages > W_CREDIT_CAP_PAGES)
			{
				w_writable_oversized = true;
				return;
			}
			if(first + pages > w_writable.size())
				w_writable.resize(first + pages + 8192, 0);
			for(uint64_t i = 0; i < pages; i++)
				if(w_writable[first + i] != 0xFF)
					w_writable[first + i]++;
		}

		// Account for an entry that is about to stop being a resident W|D
		// mapping (overwritten by insert(), or invalidated by a flush).
		inline void drop_writable(const TlbEntry& e)
		{
			if(e.generation != generation || !(e.perm & (int)TLBPermissions::PERM_W))
				return;
			const uint64_t first = e.ppage_base >> 12;
			const uint64_t pages = 1ULL << (e.page_bits - 12);
			if(pages > W_CREDIT_CAP_PAGES)
			{
				w_writable_oversized = true;
				return;
			}
			for(uint64_t i = 0; first + i < w_writable.size(); i++)
			{
				if(i >= pages)
					break;
				if(w_writable[first + i] != 0)
					w_writable[first + i]--;
			}
		}
	};
}
