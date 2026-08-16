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
#include "memory_map.hpp"
#include <cstdint>
#include <stdexcept>

namespace rv64vm::runner
{
	struct Hart;
	/**
	 * @ingroup RV64VM-API
	 * @brief RISC-V Memory Management Unit
	 */
	class MMU
	{
	  public:
		/**
		 * @brief MMU Constructor
		 */
		MMU() {}
		/**
		 * @brief MMU Destructor
		 */
		~MMU() {}

		static constexpr uint16_t _PAGE_SIZE = 4096;
		/**
		 * @brief SATP Mode
		 * @details Defines current SATP CSR register MODE bits
		 */
		enum class SatpMode : int
		{
			Bare = 0,  /** Direct access */
			Sv39 = 8,  /** Sv39 Protection mode */
			Sv48 = 9,  /** Sv48 Protection mode */
			Sv57 = 10, /** Sv57 Protection mode */
		};

		/**
		 * @brief Sv39 Protection mode namespace
		 */
		struct Sv39
		{
			static constexpr uint8_t LEVELS				= 3;
			static constexpr uint8_t PTESIZE			= 8; // bytes
			static constexpr uint8_t PPN_SHIFTS[LEVELS] = { 12, 21, 30 };
			/**
			 * @brief Sv39 Virtual address structure
			 */
			union VirtualAddress
			{
				struct
				{
					uint64_t offset : 12;
					uint64_t VPN_0 : 9;
					uint64_t VPN_1 : 9;
					uint64_t VPN_2 : 9;
					uint64_t reserved : 25;
				} fields;

				uint64_t raw;

				static constexpr uint8_t SHIFTS[3] = { 12, 21, 30 };
				static constexpr uint64_t MASKS[3] = { 0x1FF, 0x3FFFF, 0x7FFFFFF };
				const inline uint64_t get_vpn_range(int low, int high) const
				{
					const int bits = 9 * (high - low + 1);
					return (raw >> SHIFTS[low]) & ((1ULL << bits) - 1);
				}
				const inline bool isvalid() const
				{
					// Check if bits 39-63 are equal to bit 38
					int64_t shifted = static_cast<int64_t>(raw) >> 38;
					// if bit 38 == 0, then all other bits also 0
					// if bit 38 == 1, then all other bits will equal 1, but cuz we making static cast to int64_t we result in -1
					return (shifted == 0 || shifted == -1);
				}
				const inline uint64_t get_vpn(uint8_t idx) const
				{
					switch(idx)
					{
						case 0:
							return fields.VPN_0;
						case 1:
							return fields.VPN_1;
						case 2:
							return fields.VPN_2;
						default:
							throw std::logic_error("Invalid VPN index!");
					}
				}
			};
			/**
			 * @brief Sv39 Page table entry structure
			 */
			union PTE
			{
				struct
				{
					uint64_t V : 1;
					uint64_t R : 1;
					uint64_t W : 1;
					uint64_t X : 1;
					uint64_t U : 1;
					uint64_t G : 1;
					uint64_t A : 1;
					uint64_t D : 1;
					uint64_t RSW : 2;
					uint64_t PPN_0 : 9;
					uint64_t PPN_1 : 9;
					uint64_t PPN_2 : 26;
					uint64_t : 7;
					uint64_t PBMT : 2;
					uint64_t N : 1;
				} fields;

				uint64_t raw;

				const inline uint64_t get_solid_ppn() const
				{
					return (raw >> 10) & 0xFFFFFFFFFFFULL;
				}
				inline uint64_t get_ppn(uint8_t idx) const
				{
					switch(idx)
					{
						case 0:
							return fields.PPN_0;
						case 1:
							return fields.PPN_1;
						case 2:
							return fields.PPN_2;
						default:
							throw std::logic_error("Invalid PPN index!");
					}
				}
			};
		};

		/**
		 * @brief Translates Virtual address to Physical address
		 * @param Hart Pointer to Hart object
		 * @param type Access type
		 * @param va Virtual address
		 * @param pa Pointer to Physical address to set
		 * @see Hart
		 * @return Memory operation result
		 * @see MemoryReturn
		 */
		MemoryReturn translate(Hart* hart, AccessType type, uint64_t va, uint64_t* pa);

	  private:
		template <typename SvMode>
		MemoryReturn translate_impl(Hart* hart, AccessType type, uint64_t va, uint64_t* pa);

		MemoryMap* mmap;
		friend class Hart;
	};
}
