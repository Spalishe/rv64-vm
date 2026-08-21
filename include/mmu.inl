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

#include "hart.hpp"

using namespace rv64vm::runner;
inline MemoryReturn MMU::translate(Hart* hart, AccessType type, uint64_t va, uint64_t* pa)
{
	Hart::PrivilegeMode mode = hart->get_effective_mode(type);
	satp_t satp				 = hart->satp;

	uint16_t asid = satp.fields.asid;
	bool mxr	  = hart->status.fields.MXR;
	bool sum	  = hart->status.fields.SUM;
	if(tlb.lookup(va, type, asid, (int)mode, mxr, sum, pa)) [[likely]]
		return { true, 0, 0 };
	if(mode == Hart::PrivilegeMode::Machine)
	{
		*pa = va;
		return { true, 0, 0 };
	}
	switch((SatpMode)satp.fields.mode)
	{
		case SatpMode::Bare:
			*pa = va;
			return { true, 0, 0 };
		case SatpMode::Sv39:
			return translate_impl<Sv39>(hart, type, va, pa);
		default:
			unsupported_mode(hart->satp.fields.mode);
	}
}

static constexpr char AccessType_to_Fault[3]{
	EXC_LOAD_PAGE_FAULT,
	EXC_STORE_PAGE_FAULT,
	EXC_INST_PAGE_FAULT
};

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

	uint16_t asid			 = satp.fields.asid;
	Hart::PrivilegeMode mode = hart->get_effective_mode(type);

	uint64_t a = satp.fields.ppn * _PAGE_SIZE; // pte_addr
	int16_t i  = SvMode::LEVELS - 1;

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
		typename SvMode::PTE current;
		current.raw = mmap->load(addr, SvMode::PTESIZE * 8);

		bool need_update = (pte.fields.A == 0) || (type == AccessType::STORE && pte.fields.D == 0);
		if(need_update)
		{
			pte.fields.A = 1;
			if(type == AccessType::STORE)
				pte.fields.D = 1;
			mmap->store(addr, SvMode::PTESIZE * 8, pte.raw);
		}
	}

	uint64_t bit_off = 12 + i * 9;
	uint64_t vmask	 = (1ULL << bit_off) - 1; // offset inside page/superpage

	*pa = (pte.get_solid_ppn() << 12) | (raw_va & vmask);

	// Append TLB
	uint8_t perm = 0;
	if(pte.fields.R) perm |= (int)TLB::TLBPermissions::PERM_R;
	if(pte.fields.W) perm |= (int)TLB::TLBPermissions::PERM_W;
	if(pte.fields.X) perm |= (int)TLB::TLBPermissions::PERM_X;
	if(pte.fields.U) perm |= (int)TLB::TLBPermissions::PERM_U;
	if(pte.fields.A) perm |= (int)TLB::TLBPermissions::PERM_A;
	if(pte.fields.D) perm |= (int)TLB::TLBPermissions::PERM_D;
	uint8_t page_bits = 12 + i * 9; // 12 for 4K, 21 for 2M, 30 for 1G
	tlb.insert(raw_va, *pa & ~((1ULL << page_bits) - 1), page_bits,
			   perm, asid, pte.fields.G);
	return { true, 0, 0 };
}
