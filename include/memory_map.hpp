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
#include "elfparser.hpp"
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

namespace rv64vm::runner
{
	struct MemoryRegion
	{
		uint64_t base_addr;
		size_t size;
		uint8_t* data;

		MemoryRegion(uint64_t base, size_t sz)
			: base_addr(base), size(sz)
		{
			data = new uint8_t[size]{ 0 };
		}

		~MemoryRegion()
		{
			delete[] data;
		}

		uint8_t* ptr(uint64_t addr)
		{
			if(addr < base_addr || addr >= base_addr + size)
				throw std::out_of_range("Memory access out of region");
			return data + (addr - base_addr);
		}
		std::optional<uint8_t*> ptr_safe(uint64_t addr)
		{
			if(addr < base_addr || addr >= base_addr + size)
				return std::nullopt;
			return data + (addr - base_addr);
		}
	};

	struct MemoryMap
	{
		std::vector<MemoryRegion*> regions;
		MemoryRegion* ram_direct = nullptr;
		ELFParser elf			 = ELFParser(this);
		// std::unordered_map<uint64_t,MemoryRegion*> cache;

		~MemoryMap()
		{
			for(auto* r : regions)
				delete r;
		}

		void add_region(uint64_t base, size_t size)
		{
			regions.push_back(new MemoryRegion(base, size));
			if(base >= 0x80000000)
			{
				ram_direct = regions.back();
			}
		}

		bool load_file(uint64_t memory_path, std::string path = "", uint64_t* entry_pc = NULL)
		{
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if(!file.is_open())
			{
				// error
				std::cout << "[RV64-VM] File loading error! " << std::strerror(errno) << std::endl;
				return false;
			}

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);
			std::vector<char> buffer(size);
			file.read(buffer.data(), size);

			bool isElf = *(uint32_t*)buffer.data() == ELF_MAGIC;

			if(isElf)
			{
				return elf.parse(path, entry_pc);
			}
			else
			{
				// binary
				auto region	 = find_region(memory_path);
				uint8_t* ptr = region->ptr(memory_path);
				memcpy(ptr, buffer.data(), size);
				return true;
			}
		}

		bool load_buffer(uint64_t memory_path, char* buffer, uint64_t size, uint64_t* entry_pc = NULL)
		{
			bool isElf = *(uint32_t*)buffer == ELF_MAGIC;
			if(isElf)
			{
				return elf.parse(buffer, size, entry_pc);
			}
			else
			{
				auto region	 = find_region(memory_path);
				uint8_t* ptr = region->ptr(memory_path);
				memcpy(ptr, buffer, size);
				return true;
			}
		}

		uint64_t load(uint64_t addr, uint64_t size)
		{
			MemoryRegion* r = find_region(addr);
			uint8_t* p		= r->ptr(addr);
			switch(size)
			{
				case 8:
					return p[0];
				case 16:
					return p[0] | ((uint64_t)p[1] << 8);
				case 32:
					return p[0] | ((uint64_t)p[1] << 8)
						   | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24);
				case 64:
				{
					uint64_t val = 0;
					for(int i = 0; i < 8; i++)
						val |= ((uint64_t)p[i] << (i * 8));
					return val;
				}
				default:
					throw std::invalid_argument("Invalid load size");
			}
		}

		std::optional<uint64_t> load_safe(uint64_t addr, uint64_t size)
		{
			std::optional<MemoryRegion*> r = find_region_safe(addr);
			if(!r.has_value()) return std::nullopt;
			std::optional<uint8_t*> p = r.value()->ptr_safe(addr);
			if(!p.has_value()) return std::nullopt;
			switch(size)
			{
				case 8:
					return p.value()[0];
				case 16:
					return p.value()[0] | ((uint64_t)p.value()[1] << 8);
				case 32:
					return p.value()[0] | ((uint64_t)p.value()[1] << 8)
						   | ((uint64_t)p.value()[2] << 16) | ((uint64_t)p.value()[3] << 24);
				case 64:
				{
					uint64_t val = 0;
					for(int i = 0; i < 8; i++)
						val |= ((uint64_t)p.value()[i] << (i * 8));
					return val;
				}
				default:
					return std::nullopt;
			}
		}

		void copy_mem(uint64_t addr, uint64_t size, void* buffer)
		{
			MemoryRegion* r = find_region(addr);
			uint8_t* p		= r->ptr(addr);
			if((addr + size) > (r->base_addr + r->size))
				size = (r->base_addr + r->size) - addr;
			memcpy(buffer, p, size);
		}

		bool copy_mem_safe(uint64_t addr, uint64_t size, void* buffer)
		{
			std::optional<MemoryRegion*> rs = find_region_safe(addr);
			if(rs == std::nullopt) return false;
			MemoryRegion* r = *rs;
			uint8_t* p		= r->ptr(addr);
			if((addr + size) > (r->base_addr + r->size))
				size = (r->base_addr + r->size) - addr;
			memcpy(buffer, p, size);
			return true;
		}

		void store(uint64_t addr, uint64_t size, uint64_t value)
		{
			MemoryRegion* r = find_region(addr);
			uint8_t* p		= r->ptr(addr);
			switch(size)
			{
				case 8:
					p[0] = (uint8_t)value;
					break;
				case 16:
					p[0] = (uint8_t)value;
					p[1] = (uint8_t)(value >> 8);
					break;
				case 32:
					p[0] = (uint8_t)value;
					p[1] = (uint8_t)(value >> 8);
					p[2] = (uint8_t)(value >> 16);
					p[3] = (uint8_t)(value >> 24);
					break;
				case 64:
					for(int i = 0; i < 8; i++)
						p[i] = (uint8_t)(value >> (i * 8));
					break;
				default:
					throw std::invalid_argument("Invalid store size");
			}
		}

		MemoryRegion* find_region(uint64_t addr)
		{
			// if(cache.find(addr) != cache.end()) {return cache[addr];}
			if(addr >= 0x80000000) [[likely]]
				return ram_direct;
			for(auto* r : regions)
			{
				if(addr >= r->base_addr && addr < r->base_addr + r->size)
				{
					// cache[addr] = r;
					return r;
				}
			}
			throw std::out_of_range(std::format("Address not mapped in MemoryMap: 0x{:08x}", addr));
		}
		std::optional<MemoryRegion*> find_region_safe(uint64_t addr)
		{
			// if(cache.find(addr) != cache.end()) {return cache[addr];}
			for(auto* r : regions)
			{
				if(addr >= r->base_addr && addr < r->base_addr + r->size)
				{
					// cache[addr] = r;
					return r;
				}
			}
			return std::nullopt;
		}
	};
}
