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

#include "../include/tlb.hpp"
#include "../include/hart.hpp"

namespace rv64vm::runner
{
	bool TLB::lookup(uint64_t va, AccessType type, uint16_t asid, int mode, bool mxr, bool sum, uint64_t* pa)
	{
		const TlbEntry& e = entries[index(va)];

		if(e.generation != generation) [[unlikely]]
			return false;

		uint64_t page_mask = (1ULL << e.page_bits) - 1;
		if((va & ~page_mask) != e.vpage_base) [[unlikely]]
			return false;

		if(!e.global && e.asid != asid) [[unlikely]]
			return false;

		uint8_t p = e.perm;

		if(type == AccessType::STORE)
		{
			if(!(p & (int)TLBPermissions::PERM_W)) return false;
			if(!(p & (int)TLBPermissions::PERM_D)) return false; // Dirty bit
		}
		else if(type == AccessType::EXEC)
		{
			if(!(p & (int)TLBPermissions::PERM_X)) return false;
			if(mode == (int)Hart::PrivilegeMode::Supervisor && (p & (int)TLBPermissions::PERM_U)) return false;
		}
		else
		{ // LOAD
			bool can_read = (p & (int)TLBPermissions::PERM_R) || ((p & (int)TLBPermissions::PERM_X) && mxr);
			if(!can_read) return false;
		}

		bool is_u_page = p & (int)TLBPermissions::PERM_U;
		if(mode == (int)Hart::PrivilegeMode::User)
		{
			if(!is_u_page) return false;
		}
		else if(mode == (int)Hart::PrivilegeMode::Supervisor)
		{
			if(is_u_page && !sum) return false;
		}

		*pa = e.ppage_base | (va & page_mask);
		return true;
	}

	void TLB::insert(uint64_t va, uint64_t pa, uint8_t page_bits, uint8_t perm, uint16_t asid, bool global)
	{
		TlbEntry& e		   = entries[index(va)];
		uint64_t page_mask = (1ULL << page_bits) - 1;
		e				   = { va & ~page_mask, pa & ~page_mask, asid, page_bits, perm, global, generation };
	}

	__attribute__((always_inline)) inline bool TLB::check_perm(uint8_t perm, AccessType type, int mode, bool mxr, bool sum)
	{
		Hart::PrivilegeMode hmode = (Hart::PrivilegeMode)mode;
		bool u					  = perm & (int)TLB::TLBPermissions::PERM_U;

		if(hmode == Hart::PrivilegeMode::User)
		{
			if(!u) return false;
		}
		else if(hmode == Hart::PrivilegeMode::Supervisor)
		{
			if(u)
			{
				if(type == AccessType::EXEC || !sum) return false;
			}
		}

		switch(type)
		{
			case AccessType::LOAD:
				return (perm & (int)TLB::TLBPermissions::PERM_R) || ((perm & (int)TLB::TLBPermissions::PERM_X) && mxr);
			case AccessType::STORE:
				return perm & (int)TLB::TLBPermissions::PERM_W;
			case AccessType::EXEC:
				return perm & (int)TLB::TLBPermissions::PERM_X;
		}
		return false;
	}

}
