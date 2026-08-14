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

#include "../include/mmu.hpp"
#include "../include/hart.hpp"

namespace rv64vm::runner
{
	static constexpr char AccessType_to_Fault[6] = {
		EXC_LOAD_PAGE_FAULT,
		EXC_STORE_PAGE_FAULT,
		EXC_INST_PAGE_FAULT
	};
	MemoryReturn MMU::translate(Hart* hart, AccessType type, uint64_t va, uint64_t* pa)
	{
		Hart::PrivilegeMode mode = hart->get_effective_mode(type);
		if(mode == Hart::PrivilegeMode::Machine)
		{
			*pa = va;
			return { true, 0, 0 };
		}
		switch((SatpMode)hart->satp.fields.mode)
		{
			case SatpMode::Bare:
				*pa = va;
				return { true, 0, 0 };
			case SatpMode::Sv39:
				return translate_impl<Sv39>(hart, type, va, pa);
			case SatpMode::Sv48:
				throw std::logic_error("Sv48 MMU mode is not implemented.");
			case SatpMode::Sv57:
				throw std::logic_error("Sv57 MMU mode is not implemented.");
			default:
				throw std::logic_error("Unknown MMU mode: " + std::to_string(hart->satp.fields.mode));
		}
	}

	template <typename SvMode>
	uint64_t MMU::build_pa(const typename SvMode::VirtualAddress& va, const typename SvMode::PTE& pte, int leaf_level)
	{
		uint64_t pa = va.fields.offset; // lower 12 bits
		for(int lvl = 0; lvl < leaf_level; ++lvl)
			pa |= (va.get_vpn(lvl) & 0x1FF) << SvMode::PPN_SHIFTS[lvl];
		for(int lvl = leaf_level; lvl < SvMode::LEVELS; ++lvl)
			pa |= pte.get_ppn(lvl) << SvMode::PPN_SHIFTS[lvl];
		return pa;
	}

	template <typename SvMode>
	MemoryReturn MMU::translate_impl(Hart* hart, AccessType type, uint64_t raw_va, uint64_t* pa)
	{
		typename SvMode::VirtualAddress va;
		va.raw = raw_va;

		if(!va.isvalid())
		{
			*pa = 0;
			return { false, AccessType_to_Fault[(uint8_t)type], raw_va };
		}

		satp_t satp = hart->satp;

		uint16_t asid = satp.fields.asid;
		uint64_t a	  = satp.fields.ppn * _PAGE_SIZE; // pte_addr
		int16_t i	  = SvMode::LEVELS - 1;

		typename SvMode::PTE pte;
		uint64_t addr;
		while(true)
		{
			uint64_t index = va.get_vpn(i);
			addr		   = a + index * SvMode::PTESIZE;
			pte.raw		   = mmap->load(addr, SvMode::PTESIZE * 8);
			// TODO: if PMP violation then raise access fault

			if(pte.fields.V == 0 || (pte.fields.W == 1 and pte.fields.R == 0))
			{
				*pa = 0;
				return {
					false, AccessType_to_Fault[(uint8_t)type], raw_va
				};
			}
			if((pte.raw >> 54) & 0x7F)
			{
				*pa = 0;
				return {
					false, AccessType_to_Fault[(uint8_t)type], raw_va
				};
			}

			if(pte.fields.PBMT != 0)
			{
				*pa = 0;
				return {
					false, AccessType_to_Fault[(uint8_t)type], raw_va
				};
			}

			if(pte.fields.N != 0)
			{
				*pa = 0;
				return {
					false, AccessType_to_Fault[(uint8_t)type], raw_va
				};
			}
			if(pte.fields.R == 1 || pte.fields.X == 1)
			{
				break; // we reached list
			}
			i--;
			if(i < 0)
			{
				*pa = 0;
				return {
					false, AccessType_to_Fault[(uint8_t)type], raw_va
				};
			}
			a = pte.get_solid_ppn() * _PAGE_SIZE;
		}

		// leaf PTE
		if(i > 0 && (pte.get_solid_ppn() & ((1ULL << (i * 9)) - 1)) != 0)
		{
			// misaligned superpage
			*pa = 0;
			return {
				false, AccessType_to_Fault[(uint8_t)type], raw_va
			};
		}

		Hart::PrivilegeMode mode = hart->get_effective_mode(type);

		if(mode == Hart::PrivilegeMode::User)
		{
			if(!pte.fields.U)
			{
				*pa = 0;
				return {
					false, AccessType_to_Fault[(uint8_t)type], raw_va
				};
			}
		}
		else if(mode == Hart::PrivilegeMode::Supervisor)
		{
			if(pte.fields.U)
			{
				if(type == AccessType::EXEC || !hart->status.fields.SUM)
				{
					*pa = 0;
					return {
						false, AccessType_to_Fault[(uint8_t)type], raw_va
					};
				}
			}
		}
		bool allowed = false;
		if(type == AccessType::LOAD)
			allowed = pte.fields.R || (pte.fields.X && hart->status.fields.MXR);
		else if(type == AccessType::STORE)
			allowed = pte.fields.W;
		else if(type == AccessType::EXEC)
			allowed = pte.fields.X;

		if(!allowed)
		{
			*pa = 0;
			return { false, AccessType_to_Fault[(uint8_t)type], raw_va };
		}
		// update a/d bits

		if(pte.fields.A == 0 || (type == AccessType::STORE && pte.fields.D == 0))
		{
			// TODO: PMP check write PTE → access-fault
			typename SvMode::PTE current;
			current.raw = mmap->load(addr, SvMode::PTESIZE * 8);

			if(current.raw == pte.raw)
			{
				pte.fields.A = 1;
				if(type == AccessType::STORE)
					pte.fields.D = 1;
				mmap->store(addr, SvMode::PTESIZE * 8, pte.raw);
			}
			else
			{
				std::cout << "pg fault: A/D update: pte in ram is not valid with found pte" << std::endl;
				*pa = 0;
				return { false, AccessType_to_Fault[(uint8_t)type], raw_va };
			}
		}

		*pa = build_pa<SvMode>(va, pte, i);
		return { true, 0, 0 };
	}
}
