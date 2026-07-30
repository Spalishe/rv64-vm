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
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace rv64vm::runner
{
	static constexpr uint32_t ELF_MAGIC	  = 0x464C457F;
	static constexpr uint16_t ELF_RISCV	  = 0xF3;
	static constexpr uint16_t ELF_PT_LOAD = 1;

	struct MemoryMap;

	struct ELF_Header
	{
		// unsigned char e_ident[16];
		std::array<uint8_t, 16> e_ident;
		uint16_t e_type;
		uint16_t e_machine;
		uint32_t e_version;
		uint64_t e_entry;
		uint64_t e_phoff;
		uint64_t e_shoff;
		uint32_t e_flags;
		uint16_t e_ehsize;
		uint16_t e_phentsize;
		uint16_t e_phnum;
		uint16_t e_shentsize;
		uint16_t e_shnum;
		uint16_t e_shstrndx;
	};
	struct ELF_ProgramHeader
	{
		uint32_t p_type;
		uint32_t p_flags;
		uint64_t p_offset;
		uint64_t p_vaddr;
		uint64_t p_paddr;
		uint64_t p_filesz;
		uint64_t p_memsz;
		uint64_t p_align;
	};
	struct ELF_SectionHeader
	{
		uint32_t sh_name;
		uint32_t sh_type;
		uint64_t sh_flags;
		uint64_t sh_addr;
		uint64_t sh_offset;
		uint64_t sh_size;
		uint32_t sh_link;
		uint32_t sh_info;
		uint64_t sh_addralign;
		uint64_t sh_entsize;
	};

	struct ELFParser
	{
		ELFParser(MemoryMap* mmap);
		MemoryMap* mmap;

		bool parse(std::string file, uint64_t* entry_pc);
		bool parse(char* buffer, size_t size, uint64_t* entry_pc);

		template <typename T>
		T read_from_buffer(const char* data, size_t* offset)
		{
			T val;
			std::memcpy(&val, data + *offset, sizeof(T));
			*offset += sizeof(T);
			return val;
		}
	};
}
