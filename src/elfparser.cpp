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
#include <algorithm>
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

		auto& regions = mmap->get_regions();

		for(int i = 0; i < header.e_phnum; i++)
		{
			auto& ph = pheaders[i];
			if(ph.p_type != ELF_PT_LOAD)
				continue;

			const uint64_t seg_start = ph.p_vaddr;
			const uint64_t seg_end	 = seg_start + ph.p_memsz;

			bool covered = false;

			for(auto* reg : regions)
			{
				const uint64_t reg_start = reg->get_base_addr();
				const uint64_t reg_end	 = reg->get_base_addr() + reg->get_size();

				const uint64_t ov_start = std::max(seg_start, reg_start);
				const uint64_t ov_end	= std::min(seg_end, reg_end);

				if(ov_start >= ov_end)
					continue;

				covered = true;

				const uint64_t file_off = ph.p_offset + (ov_start - seg_start);
				const uint64_t avail	= ph.p_offset + ph.p_filesz - file_off;
				const uint64_t copy_len = std::min(ov_end - ov_start, avail);

				memcpy(reg->get_data() + (ov_start - reg_start), buffer + file_off, copy_len);

				if(ov_end - ov_start > copy_len)
					memset(reg->get_data() + (ov_start - reg_start) + copy_len,
						   0,
						   (ov_end - ov_start) - copy_len);
			}

			if(!covered)
				mmap->add_region(ph.p_vaddr, ph.p_memsz);
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
