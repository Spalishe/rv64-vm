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

#include "../include/elfparser.hpp"
#include "../include/memory_map.hpp"
#include <vector>

namespace rv64vm::runner
{
	ELFParser::ELFParser(MemoryMap* mmap) : mmap(mmap) {

											};

	bool ELFParser::parse(char* buffer, size_t size, uint64_t* entry_pc)
	{
		uint64_t offset;

		ELF_Header header = read_from_buffer<ELF_Header>(buffer, &offset);

		uint32_t v32;
		uint32_t v8;
		std::memcpy(&v32, header.e_ident.data(), sizeof(uint32_t));
		if(v32 != ELF_MAGIC)
		{
			printf("[ELF] File is not an elf!");
			return false;
		}
		std::memcpy(&v8, header.e_ident.data() + 4, sizeof(uint8_t));
		if(v8 == 1)
		{
			printf("[ELF] 32-bit applications not supported!");
			return false;
		}

		if(header.e_machine != ELF_RISCV)
		{
			printf("[ELF] ELF Architecture isn't RISC-V");
			return false;
		}

		if(entry_pc != NULL)
		{
			*entry_pc = header.e_entry;
		}

		offset = header.e_phoff;
		std::vector<ELF_ProgramHeader> pheaders;
		std::vector<ELF_SectionHeader> sheaders;
		for(int i = 0; i < header.e_phnum; i++)
		{
			pheaders.push_back(read_from_buffer<ELF_ProgramHeader>(buffer, &offset));
		}
		offset = header.e_shoff;
		for(int i = 0; i < header.e_shnum; i++)
		{
			sheaders.push_back(read_from_buffer<ELF_SectionHeader>(buffer, &offset));
		}

		for(int i = 0; i < header.e_phnum; i++)
		{
			auto& ph = pheaders[i];
			if(ph.p_type != ELF_PT_LOAD)
				continue;

			if(ph.p_vaddr < 0x80000000) mmap->add_region(ph.p_vaddr, ph.p_memsz);
			MemoryMap::MemoryRegion* newreg = mmap->find_region(ph.p_vaddr);

			memcpy(newreg->get_data() + (ph.p_paddr - newreg->get_base_addr()), buffer + ph.p_offset, ph.p_filesz);

			memset(newreg->get_data() + ph.p_filesz,
				   0,
				   ph.p_memsz - ph.p_filesz);
		}

		return true;
	}

	bool ELFParser::parse(std::string file_path, uint64_t* entry_pc)
	{
		std::ifstream file(file_path, std::ios::binary | std::ios::ate);
		if(!file.is_open())
		{
			// error
			printf("[RV64-VM] File loading error! %s\n", std::strerror(errno));
			return false;
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		char* buffer = new char[size]{};
		file.read(buffer, size);
		return parse(buffer, size, entry_pc);
	}
}
