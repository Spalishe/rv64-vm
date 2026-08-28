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

#include "../include/mmio.hpp"
#include "../include/hart.hpp"

namespace rv64vm::runner
{
	MMIO::MMIO(MemoryMap* mmap, uint64_t mem_size) : mmap(mmap), memsize(mem_size) {};

	MemoryReturn MMIO::write(
		Hart& h,
		uint64_t addr,
		MemorySize size,
		uint64_t val,
		bool isphys)
	{
		const uint64_t access_size = (uint64_t)size;

		const bool paging = !isphys && h.satp.fields.mode != 0;

		const bool cross_page = paging && ((addr & 0xFFFULL) + access_size > 0x1000ULL);

		// fast path
		if(!cross_page)
		{
			uint64_t paddr;

			if(!isphys) [[likely]]
			{
				auto res = h.get_mmu().translate(
					&h,
					AccessType::STORE,
					addr,
					&paddr);

				if(!res.is_success)
					return res;
			}
			else
			{
				paddr = addr;
			}

			const uint64_t end = 0x80000000ULL + memsize;

			if(paddr >= 0x80000000ULL && paddr <= end - access_size) [[likely]]
			{
				// DRAM
				h.amo_check_reservation(paddr);
				write_dram_fast(paddr, size, val);
				return { true, 0, 0 };
			}

			// Looking up for devices in this range
			for(const auto& dev : devs)
			{
				if(paddr >= dev->start && paddr <= dev->start + dev->size - access_size)
				{
					h.amo_check_reservation(paddr);
					dev->write(
						paddr,
						size,
						val);

					return { true, 0, 0 };
				}
			}

			return { false, EXC_STORE_ACCESS_FAULT, addr };
		}

		// cross-page access
		const uint64_t first_size = 0x1000ULL - (addr & 0xFFFULL);

		const uint64_t second_size = access_size - first_size;

		uint64_t first_paddr;
		auto first_res = h.get_mmu().translate(
			&h,
			AccessType::STORE,
			addr,
			&first_paddr);

		if(!first_res.is_success)
			return first_res;

		uint64_t second_paddr;
		auto second_res = h.get_mmu().translate(
			&h,
			AccessType::STORE,
			addr + first_size,
			&second_paddr);

		if(!second_res.is_success)
			return second_res;

		// first physical fragment, low bytes of val belong to the first page
		const uint64_t first_val = val & ((1ULL << (first_size * 8)) - 1ULL);

		const uint64_t first_end = 0x80000000ULL + memsize;

		if(first_paddr >= 0x80000000ULL && first_paddr <= first_end - first_size)
		{
			h.amo_check_reservation(first_paddr);

			write_dram_fast(first_paddr, (MemorySize)first_size, first_val);
		}
		else
		{
			bool handled = false;

			for(const auto& dev : devs)
			{
				if(first_paddr >= dev->start && first_paddr <= dev->start + dev->size - first_size)
				{
					h.amo_check_reservation(first_paddr);

					dev->write(
						first_paddr,
						(MemorySize)first_size,
						first_val);

					handled = true;
					break;
				}
			}

			if(!handled)
				return { false, EXC_STORE_ACCESS_FAULT, addr };
		}

		// second physical fragment, remaining high bytes of val belong to the next page
		const uint64_t second_val = val >> (first_size * 8);

		if(second_paddr >= 0x80000000ULL && second_paddr <= first_end - second_size)
		{
			h.amo_check_reservation(second_paddr);

			write_dram_fast(second_paddr, (MemorySize)second_size, second_val);
		}
		else
		{
			bool handled = false;

			for(const auto& dev : devs)
			{
				if(second_paddr >= dev->start && second_paddr <= dev->start + dev->size - second_size)
				{
					h.amo_check_reservation(second_paddr);

					dev->write(
						second_paddr,
						(MemorySize)second_size,
						second_val);

					handled = true;
					break;
				}
			}

			if(!handled)
				return {
					false,
					EXC_STORE_ACCESS_FAULT,
					addr + first_size
				};
		}

		return { true, 0, 0 };
	}

	inline uint64_t MMIO::read_dram_fast(
		uint64_t paddr,
		MemorySize size)
	{
		if(direct_ram == nullptr) [[unlikely]]
			direct_ram = mmap->get_ram_direct()->get_data();

		unsigned char* ptr = direct_ram + (paddr - 0x80000000ULL);

		switch(size)
		{
			case MemorySize::Byte:
				return *(uint8_t*)ptr;

			case MemorySize::Short:
				return *(uint16_t*)ptr;

			case MemorySize::Int:
				return *(uint32_t*)ptr;

			case MemorySize::Long:
				return *(uint64_t*)ptr;
		}

		return 0;
	}

	inline void MMIO::write_dram_fast(
		uint64_t paddr,
		MemorySize size,
		uint64_t val)
	{
		if(direct_ram == nullptr) [[unlikely]]
			direct_ram = mmap->get_ram_direct()->get_data();

		smc_store_hit(paddr);

		unsigned char* ptr = direct_ram + (paddr - 0x80000000ULL);

		switch(size)
		{
			case MemorySize::Byte:
				*(uint8_t*)ptr	  = (uint8_t)val;
				break;

			case MemorySize::Short:
				*(uint16_t*)ptr	  = (uint16_t)val;
				break;

			case MemorySize::Int:
				*(uint32_t*)ptr	  = (uint32_t)val;
				break;

			case MemorySize::Long:
				*(uint64_t*)ptr	  = (uint64_t)val;
				break;
		}
	}

	MemoryReturn MMIO::read(
		Hart& h,
		uint64_t addr,
		MemorySize size,
		void* val,
		bool isphys)
	{
		const uint64_t access_size = (uint64_t)size;

		const bool paging = !isphys && h.satp.fields.mode != 0;

		const bool cross_page = paging && ((addr & 0xFFFULL) + access_size > 0x1000ULL);

		// fast path
		if(!cross_page)
		{
			uint64_t paddr;

			if(!isphys) [[likely]]
			{
				auto res = h.get_mmu().translate(
					&h,
					AccessType::LOAD,
					addr,
					&paddr);

				if(!res.is_success)
					return res;
			}
			else
			{
				paddr = addr;
			}

			uint64_t out;

			const uint64_t end = 0x80000000ULL + memsize;

			if(paddr >= 0x80000000ULL && paddr <= end - access_size) [[likely]]
			{
				out = read_dram_fast(
					paddr,
					size);

				goto success;
			}

			for(const auto& dev : devs)
			{
				if(paddr >= dev->start && paddr <= dev->start + dev->size - access_size)
				{
					out = dev->read(
						paddr,
						size);

					goto success;
				}
			}

			return { false, EXC_LOAD_ACCESS_FAULT, addr };

		success:
			switch(size)
			{
				case MemorySize::Byte:
					*(uint8_t*)val = out;
					break;

				case MemorySize::Short:
					*(uint16_t*)val = out;
					break;

				case MemorySize::Int:
					*(uint32_t*)val = out;
					break;

				case MemorySize::Long:
					*(uint64_t*)val = out;
					break;
			}

			return { true, 0, 0 };
		}

		// cross-page access
		const uint64_t first_size = 0x1000ULL - (addr & 0xFFFULL);

		const uint64_t second_size = access_size - first_size;

		uint64_t first_paddr;

		auto first_res = h.get_mmu().translate(
			&h,
			AccessType::LOAD,
			addr,
			&first_paddr);

		if(!first_res.is_success)
			return first_res;

		uint64_t second_paddr;

		auto second_res = h.get_mmu().translate(
			&h,
			AccessType::LOAD,
			addr + first_size,
			&second_paddr);

		if(!second_res.is_success)
			return second_res;

		uint64_t first_out;
		uint64_t second_out;

		const uint64_t end = 0x80000000ULL + memsize;

		// first fragment
		if(first_paddr >= 0x80000000ULL && first_paddr <= end - first_size)
		{
			first_out = read_dram_fast(
				first_paddr,
				(MemorySize)first_size);
		}
		else
		{
			bool handled = false;

			for(const auto& dev : devs)
			{
				if(first_paddr >= dev->start && first_paddr <= dev->start + dev->size - first_size)
				{
					first_out = dev->read(
						first_paddr,
						(MemorySize)first_size);

					handled = true;
					break;
				}
			}

			if(!handled)
				return { false, EXC_LOAD_ACCESS_FAULT, addr };
		}

		// second fragment
		if(second_paddr >= 0x80000000ULL && second_paddr <= end - second_size)
		{
			second_out = read_dram_fast(
				second_paddr,
				(MemorySize)second_size);
		}
		else
		{
			bool handled = false;

			for(const auto& dev : devs)
			{
				if(second_paddr >= dev->start && second_paddr <= dev->start + dev->size - second_size)
				{
					second_out = dev->read(
						second_paddr,
						(MemorySize)second_size);

					handled = true;
					break;
				}
			}

			if(!handled)
			{
				return {
					false,
					EXC_LOAD_ACCESS_FAULT,
					addr + first_size
				};
			}
		}

		// reassemble result
		const uint64_t out = first_out | (second_out << (first_size * 8));

		switch(size)
		{
			case MemorySize::Byte:
				*(uint8_t*)val = out;
				break;

			case MemorySize::Short:
				*(uint16_t*)val = out;
				break;

			case MemorySize::Int:
				*(uint32_t*)val = out;
				break;

			case MemorySize::Long:
				*(uint64_t*)val = out;
				break;
		}

		return { true, 0, 0 };
	}
}
