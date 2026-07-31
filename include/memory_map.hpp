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
	/**
	 * @ingroup RV64VM-API
	 * @brief RV64-VM Memory map class.
	 * @details This class implements Main memory storage.
	 */
	class MemoryMap
	{
	  public:
		/**
		 * @brief MemoryMap constructor
		 */
		MemoryMap() {};
		/**
		 * @brief MemoryMap destructor
		 * @details Safely removes all regions
		 */
		~MemoryMap()
		{
			for(auto* r : regions)
				delete r;
		}

		/**
		 * @brief Memory Region object
		 * @details A pie in a total cake. Contains raw data to memory.
		 */
		class MemoryRegion
		{
		  public:
			/**
			 * @brief Returns Memory region base address in GUEST ram
			 * @return Guest base address
			 */
			uint64_t get_base_addr() const { return base_addr; }
			/**
			 * @brief Returns Memory region size.
			 * @return Size (bytes)
			 */
			size_t get_size() const { return size; }

			/**
			 * @brief Returns Host pointer to Guest address.
			 * @param addr Guest address
			 * @return Host address
			 * @note Make sure you check if MemoryRegion base address and size is in range
			 */
			uint8_t* ptr(uint64_t addr)
			{
				if(addr < base_addr || addr >= base_addr + size)
					throw std::out_of_range("Memory access out of region");
				return data + (addr - base_addr);
			}

		  private:
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
			friend class MemoryMap;
		};

		/**
		 * @brief Returns all created regions
		 * @see MemoryRegion
		 * @return Memory regions
		 */
		inline std::vector<MemoryRegion*>& get_regions() { return regions; }
		/**
		 * @brief Returns pointer to 0x80000000 guest ram region
		 * @see MemoryRegion
		 * @return Direct pointer to region
		 */
		inline MemoryRegion* get_ram_direct() const { return ram_direct; }
		/**
		 * @brief Creates new guest region in memory
		 * @param base Guest base
		 * @param size Region size
		 */
		void add_region(uint64_t base, size_t size)
		{
			regions.push_back(new MemoryRegion(base, size));
			if(base >= 0x80000000)
			{
				ram_direct = regions.back();
			}
		}
		/**
		 * @brief Loads binary or ELF file to guest memory
		 * @param memory_path Guest address to put data in
		 * @param path Path to file
		 * @param entry_pc Output program counter readed from ELF file(if you use elf)
		 * @return Success boolean
		 */
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
		/**
		 * @brief Loads buffer to guest memory
		 * @param memory_path Guest address to put data in
		 * @param buffer Host pointer to buffer
		 * @param size Buffer size
		 * @param entry_pc Output program counter readed from ELF file(if you use elf)
		 * @return Success boolean
		 */
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
		/**
		 * @brief Loads value from guest memory
		 * @param addr Guest address
		 * @param size Data size (bits)
		 * @return Value stored in memory
		 */
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
		/**
		 * @brief Stores value to guest memory
		 * @param addr Guest address
		 * @param size Data size (bits)
		 * @param value Value to store
		 */
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

		/**
		 * @brief Finds region by specidied guest address
		 * @param addr Guest addr
		 * @return Memory region
		 * @see MemoryRegion
		 */
		MemoryRegion* find_region(uint64_t addr)
		{
			// if(cache.find(addr) != cache.end()) {return cache[addr];}
			if(addr >= 0x80000000) [[likely]]
				return ram_direct;
			for(auto* r : regions)
			{
				if(addr >= r->get_base_addr() && addr < r->get_base_addr() + r->get_size())
				{
					// cache[addr] = r;
					return r;
				}
			}
			throw std::out_of_range(std::format("Address not mapped in MemoryMap: 0x{:08x}", addr));
		}

	  private:
		std::vector<MemoryRegion*> regions;
		MemoryRegion* ram_direct = nullptr;
		ELFParser elf			 = ELFParser(this);
	};
}
