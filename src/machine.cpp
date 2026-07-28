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

#include "../include/machine.hpp"
#include "../include/defines/rvem.hpp"
#include "../include/devices/clint.hpp"
#include "../include/devices/i2c/i2c-core.hpp"
#include "../include/devices/plic.hpp"
#include "../include/devices/syscon.hpp"
#include "../include/devices/uart.hpp"
#include "../include/devices/virtio_blk.hpp"

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <vector>

Machine::Machine(uint64_t mem_size, uint8_t hart_count) : memory_size(mem_size), harts_count(hart_count)
{
	// Init harts
	for(int i = 0; i < hart_count; i++)
	{
		harts.emplace_back(i, mem_size);
	}
};

void Machine::init_fdt()
{
	fdt = fdt_node_create(NULL);
	fdt_node_add_prop_u32(fdt, "#address-cells", 2);
	fdt_node_add_prop_u32(fdt, "#size-cells", 2);
	fdt_node_add_prop_str(fdt, "model", RVEM_VERSION);
	fdt_node_add_prop(fdt, "compatible", "riscv-virtio\0simple-bus\0", 24);

	// chosen
	fdt_node* chosen = fdt_node_create("chosen");
	std::stringstream rng_seed;
	for(int i = 0; i < 16; i++)
	{
		char buf[12];
		std::srand(std::time({}));
		uint32_t rand = std::rand();
		snprintf(buf, sizeof(buf), "0x%08x ", rand);
		rng_seed << buf;
	}
	rng_seed << std::endl;
	fdt_node_add_prop_str(chosen, "rng-seed", rng_seed.str().c_str()); // why there is no direct conversion from stringstream to c string?
	if(append.length() != 0)
	{
		fdt_node_add_prop_str(chosen, "bootargs", append.c_str());
	}
	fdt_node_add_prop_str(chosen, "stdout-path", "/soc/uart@10000000"); // i suggest we have uart at all times
	fdt_node_add_child(fdt, chosen);

	// memory
	fdt_node* memory = fdt_node_create_reg("memory", 0x80000000);
	fdt_node_add_prop_str(memory, "device_type", "memory");
	std::vector<uint32_t>
		memcells = { 0x0, 0x80000000, (uint32_t)(memory_size >> 32), (uint32_t)(memory_size & 0xFFFFFFFF) };
	fdt_node_add_prop_cells(memory, "reg", memcells, 4);
	fdt_node_add_child(fdt, memory);

	// cpus
	fdt_node* cpus = fdt_node_create("cpus");
	fdt_node_add_prop_u32(cpus, "#address-cells", 1);
	fdt_node_add_prop_u32(cpus, "#size-cells", 0);
	fdt_node_add_prop_u32(cpus, "timebase-frequency", timebase);
	for(int i = 0; i < harts_count; i++)
	{
		Hart& hart	  = harts[i];
		fdt_node* cpu = fdt_node_create_reg("cpu", hart.id);
		fdt_node_add_prop_str(cpu, "device_type", "cpu");
		fdt_node_add_prop_u32(cpu, "reg", hart.id);
		fdt_node_add_prop_u32(cpu, "riscv,cboz-block-size", 64);
		fdt_node_add_prop_str(cpu, "compatible", "riscv");
		fdt_node_add_prop_str(cpu, "riscv,isa", "rv64imafdc_zicsr_zifencei_zicboz_zba_zbb_zbc_zbs");
		fdt_node_add_prop_str(cpu, "mmu-type", "riscv,none");
		fdt_node_add_prop_str(cpu, "status", "okay");

		fdt_node* intc = fdt_node_create("interrupt-controller");
		fdt_node_add_prop_u32(intc, "#interrupt-cells", 0x1);
		fdt_node_add_prop(intc, "interrupt-controller", NULL, 0);
		fdt_node_add_prop_str(intc, "compatible", "riscv,cpu-intc");
		fdt_node_get_phandle(intc);

		fdt_node_add_child(cpu, intc);
		fdt_node_add_child(cpus, cpu);

		// Add phandles
		fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt, "cpus"), "cpu", i));
		fdt_node_get_phandle(fdt_node_find(fdt_node_find_reg(fdt_node_find(fdt, "cpus"), "cpu", i), "interrupt-controller"));
	}
	fdt_node_add_child(fdt, cpus);

	fdt_node* soc = fdt_node_create("soc");
	fdt_node_add_prop_u32(soc, "#address-cells", 2);
	fdt_node_add_prop_u32(soc, "#size-cells", 2);
	fdt_node_add_prop_str(soc, "compatible", "simple-bus");
	fdt_node_add_prop(soc, "ranges", NULL, 0);
	fdt_node_add_child(fdt, soc);
}

void Machine::write_fdt()
{
	if(dtb_file != nullptr)
	{
		load_fdt();
		return;
	}
	uint64_t dtb_path_in_memory = 0x80000000 + memory_size - 0x20000;
	size_t dtb_size				= fdt_size(fdt);
	void* buffer				= malloc(dtb_size);

	size_t size = fdt_serialize(fdt, buffer, 0x1000, 0);

	if(dtb_dump_path.length() != 0)
	{
		FILE* f = fopen(dtb_dump_path.c_str(), "wb");
		fwrite(buffer, 1, size, f);
		fclose(f);
	}

	char* bytes = static_cast<char*>(buffer);
	mmap->load_buffer(dtb_path_in_memory, bytes, size);

	free(buffer);
}

void Machine::load_fdt()
{
	uint64_t dtb_path_in_memory = 0x80000000 + memory_size - 0x20000;

	fseek(dtb_file, 0, SEEK_END);
	long size = ftell(dtb_file);
	fseek(dtb_file, 0, SEEK_SET);
	char* buffer = new char[size + 1];
	fread(buffer, 1, size, dtb_file);
	buffer[size] = '\0';

	mmap->load_buffer(dtb_path_in_memory, buffer, size);
}

void Machine::init_mmap()
{
	mmap = new MemoryMap();
	mmap->add_region(0x80000000, memory_size);
	mmio = new MMIO(mmap, memory_size);
	idec = new InstructionDecoder();
	idec->init_all_instrs();
}

void Machine::init_auto_devices()
{
	mmio->create_device_auto<PLIC>(*this);
	auto ptr		= mmio->create_device_auto<UART>(*this);
	ptr->out_stream = uart_out;
	mmio->create_device_auto<CLINT>(*this);
	mmio->create_device_auto<SYSCON>(*this);
	mmio->create_device_auto<I2C>(*this);
	if(image_file != nullptr)
	{
		mmio->create_device_auto<VirtIO_BLK>(*this);
	}
}

void Machine::run()
{
	fseek(bios_file, 0, SEEK_END);
	long size = ftell(bios_file);
	fseek(bios_file, 0, SEEK_SET);
	char* buffer = new char[size + 1];
	fread(buffer, 1, size, bios_file);
	buffer[size] = '\0';
	bool out	 = mmap->load_buffer(0x80000000, buffer, size, &entry_pc);
	delete[] buffer;
	if(!out) return;
	if(kernel_file != nullptr)
	{
		fseek(kernel_file, 0, SEEK_END);
		long size = ftell(kernel_file);
		fseek(kernel_file, 0, SEEK_SET);
		char* buffer = new char[size + 1];
		fread(buffer, 1, size, kernel_file);
		buffer[size] = '\0';
		out			 = mmap->load_buffer(0x80200000, buffer, size);
		delete[] buffer;
	}
	if(!out) return;

	uint64_t dtb_path_in_memory = 0x80000000 + memory_size - 0x20000;
	// init all harts
	for(int i = 0; i < harts_count; i++)
	{
		Hart& h = harts[i];
		h.mmap	= mmap;
		h.mmio	= mmio;
		h.idec	= idec;

		h.init(dtb_path_in_memory, entry_pc);
	}

// prepare
#ifdef USE_GDBSTUB
	state = gdb ? MachineState::Halted : MachineState::Running;
#else
	state = MachineState::Running;
#endif

	// create work thread
	work_thread	  = std::thread(&Machine::work, this);
	work_thread_w = true;
}

void Machine::work()
{
	while(state.load(std::memory_order_acquire) != MachineState::Off)
	{
		if(state.load(std::memory_order_acquire) == MachineState::Resetting)
		{
			// Total machine reset

			destroy_harts();
			reset_memory();

			fseek(bios_file, 0, SEEK_END);
			long size = ftell(bios_file);
			fseek(bios_file, 0, SEEK_SET);
			char* buffer = new char[size + 1];
			fread(buffer, 1, size, bios_file);
			buffer[size] = '\0';
			mmap->load_buffer(0x80000000, buffer, size);
			delete[] buffer;
			if(kernel_file != nullptr)
			{
				fseek(kernel_file, 0, SEEK_END);
				long size = ftell(kernel_file);
				fseek(kernel_file, 0, SEEK_SET);
				char* buffer = new char[size + 1];
				fread(buffer, 1, size, kernel_file);
				buffer[size] = '\0';
				mmap->load_buffer(0x80200000, buffer, size);
				delete[] buffer;
			}

			write_fdt();

			// Init harts
			uint64_t dtb_path_in_memory = 0x80000000 + memory_size - 0x20000;
			for(int i = 0; i < harts_count; i++)
			{
				harts.emplace_back(i, memory_size);
				Hart& hart = harts.back();
				hart.mmap  = mmap;
				hart.mmio  = mmio;
				hart.idec  = idec;

				hart.init(dtb_path_in_memory, entry_pc);
			}
// prepare
#ifdef USE_GDBSTUB
			state.store(gdb ? MachineState::Halted : MachineState::Running, std::memory_order_release);
#else
			state.store(MachineState::Running, std::memory_order_release);
#endif
			continue;
		}

		if(state.load(std::memory_order_acquire) == MachineState::Halted)
		{
#ifdef USE_GDBSTUB
			if(gdb_single_step)
			{
				state = MachineState::Running;
			}
#endif
			std::this_thread::yield();
			continue;
		}

		// Update devices
		dev_tick_time++;
		if(dev_tick_time == 0x1000)
		{
			dev_tick_time = 0;
			mmio->tick_all();
		}
		// Update harts
		for(int i = 0; i < harts_count; i++)
		{
			Hart& h = harts[i];
			h.tick();
		}

#ifdef USE_GDBSTUB
		if(gdb_single_step)
		{
			gdb_single_step = false;
			state			= MachineState::Halted;
		}
#endif
	}
	work_thread_w = false;
}

void Machine::stop()
{
	// stop work thread if it exists
	if(work_thread_w)
	{
		state.store(MachineState::Off, std::memory_order_release);
		if(!work_thread_joined && std::this_thread::get_id() != work_thread.get_id())
			work_thread.join();
	}
}

void Machine::reset()
{
	if(state.load() == MachineState::Off)
	{
		destroy_harts();
		reset_memory();

		fseek(bios_file, 0, SEEK_END);
		long size = ftell(bios_file);
		fseek(bios_file, 0, SEEK_SET);
		char* buffer = new char[size + 1];
		fread(buffer, 1, size, bios_file);
		buffer[size] = '\0';
		mmap->load_buffer(0x80000000, buffer, size);
		delete[] buffer;
		if(kernel_file != nullptr)
		{
			fseek(kernel_file, 0, SEEK_END);
			long size = ftell(kernel_file);
			fseek(kernel_file, 0, SEEK_SET);
			char* buffer = new char[size + 1];
			fread(buffer, 1, size, kernel_file);
			buffer[size] = '\0';
			mmap->load_buffer(0x80200000, buffer, size);
			delete[] buffer;
		}

		write_fdt();

		// Init harts
		uint64_t dtb_path_in_memory = 0x80000000 + memory_size - 0x20000;
		for(int i = 0; i < harts_count; i++)
		{
			harts.emplace_back(i, memory_size);
			Hart& hart = harts.back();
			hart.mmap  = mmap;
			hart.mmio  = mmio;
			hart.idec  = idec;

			hart.init(dtb_path_in_memory, entry_pc);
		}
// prepare
#ifdef USE_GDBSTUB
		state.store(gdb ? MachineState::Halted : MachineState::Running, std::memory_order_release);
#else
		state.store(MachineState::Running, std::memory_order_release);
#endif
	}
	else
		state.store(MachineState::Resetting, std::memory_order_release);
}

void Machine::reset_memory()
{
	for(auto* reg : mmap->regions)
	{
		memset(reg->data, 0, reg->size);
	}
}

void Machine::destroy_harts()
{
	harts.clear();
}
void Machine::destroy_devices()
{
	// If you think this will not remove actual objects from heap - it will
	mmio->devs.clear();
}
void Machine::destroy_mmap()
{
	for(auto* reg : mmap->regions)
	{
		delete reg;
	}
	mmap->regions.clear();
	delete mmap;
	delete mmio;
}
