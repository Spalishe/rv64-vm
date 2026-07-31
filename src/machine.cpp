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

namespace rv64vm::runner
{
	Machine::Machine(const MachineConfig& cfg) : config(cfg)
	{
		// Init harts
		harts.reserve(config.hart_count);
		for(int i = 0; i < config.hart_count; i++)
		{
			harts.emplace_back(i, config.memory_size);
		}
		init_mmap();
	};
	Machine::~Machine()
	{
		stop();
		destroy_harts();
		destroy_devices();
		destroy_mmap();

		if(image_file) fclose(image_file);
		if(bios_file) fclose(bios_file);
		if(kernel_file) fclose(kernel_file);
		if(dtb_file) fclose(dtb_file);

		if(fdt) fdt_node_free(fdt);
		delete idec;
	}

	bool Machine::load_image(const std::string& path)
	{
		if(image_file) fclose(image_file);
		image_file = fopen(path.c_str(), "r+b");
		return image_file != nullptr;
	}

	bool Machine::load_bios(const std::string& path)
	{
		if(bios_file) fclose(bios_file);
		bios_file = fopen(path.c_str(), "rb");
		return bios_file != nullptr;
	}

	bool Machine::load_kernel(const std::string& path)
	{
		if(kernel_file) fclose(kernel_file);
		kernel_file = fopen(path.c_str(), "rb");
		return kernel_file != nullptr;
	}

	bool Machine::load_dtb(const std::string& path)
	{
		if(dtb_file) fclose(dtb_file);
		dtb_file = fopen(path.c_str(), "rb");
		return dtb_file != nullptr;
	}

	FILE* Machine::get_image()
	{
		return image_file ? image_file : nullptr;
	}

	void Machine::set_uart_output(FILE* stream)
	{
		if(stream) uart_out = stream;
	}
	FILE* Machine::get_uart_output()
	{
		return uart_out;
	}

#ifdef USE_GDBSTUB
	void Machine::enable_gdb(bool enable, uint16_t port)
	{
		gdb_port = port;
		gdb		 = enable;
	}
	void Machine::set_gdb_single_step(bool single_step)
	{
		gdb_single_step = single_step;
	}
#endif

	void Machine::init_fdt()
	{
		fdt = fdt_node_create(NULL);
		fdt_node_add_prop_u32(fdt, "#address-cells", 2);
		fdt_node_add_prop_u32(fdt, "#size-cells", 2);
		fdt_node_add_prop_str(fdt, "model", RVEM_VERSION);
		fdt_node_add_prop(fdt, "compatible", "riscv-virtio\0simple-bus\0", 24);

		fdt_node* chosen = fdt_node_create("chosen");
		std::stringstream rng_seed;
		std::srand(static_cast<unsigned int>(std::time(nullptr)));

		for(int i = 0; i < 16; i++)
		{
			char buf[12];
			uint32_t rand_val = static_cast<uint32_t>(std::rand());
			snprintf(buf, sizeof(buf), "0x%08x ", rand_val);
			rng_seed << buf;
		}
		rng_seed << std::endl;

		fdt_node_add_prop_str(chosen, "rng-seed", rng_seed.str().c_str());
		if(!config.append.empty())
		{
			fdt_node_add_prop_str(chosen, "bootargs", config.append.c_str());
		}
		fdt_node_add_prop_str(chosen, "stdout-path", "/soc/uart@10000000");
		fdt_node_add_child(fdt, chosen);

		fdt_node* memory = fdt_node_create_reg("memory", 0x80000000);
		fdt_node_add_prop_str(memory, "device_type", "memory");
		std::vector<uint32_t> memcells = {
			0x0, 0x80000000,
			static_cast<uint32_t>(config.memory_size >> 32),
			static_cast<uint32_t>(config.memory_size & 0xFFFFFFFF)
		};
		fdt_node_add_prop_cells(memory, "reg", memcells, 4);
		fdt_node_add_child(fdt, memory);

		fdt_node* cpus = fdt_node_create("cpus");
		fdt_node_add_prop_u32(cpus, "#address-cells", 1);
		fdt_node_add_prop_u32(cpus, "#size-cells", 0);
		fdt_node_add_prop_u32(cpus, "timebase-frequency", config.timebase);

		for(int i = 0; i < config.hart_count; i++)
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
		if(!config.init_fdt) return;
		uint64_t dtb_path_in_memory = 0x80000000 + config.memory_size - 0x20000;
		size_t dtb_size				= fdt_size(fdt);
		void* buffer				= malloc(dtb_size);

		size_t size = fdt_serialize(fdt, buffer, 0x1000, 0);

		if(!config.dtb_dump_path.empty())
		{
			FILE* f = fopen(config.dtb_dump_path.c_str(), "wb");
			if(f)
			{
				fwrite(buffer, 1, size, f);
				fclose(f);
			}
		}

		char* bytes = static_cast<char*>(buffer);
		mmap->load_buffer(dtb_path_in_memory, bytes, size);
		free(buffer);
	}

	void Machine::load_fdt()
	{
		if(!dtb_file) return;
		uint64_t dtb_path_in_memory = 0x80000000 + config.memory_size - 0x20000;

		fseek(dtb_file, 0, SEEK_END);
		long size = ftell(dtb_file);
		fseek(dtb_file, 0, SEEK_SET);

		std::vector<char> buffer(size + 1, '\0');
		fread(buffer.data(), 1, size, dtb_file);

		mmap->load_buffer(dtb_path_in_memory, buffer.data(), size);
	}

	void Machine::init_mmap()
	{
		mmap = new MemoryMap();
		mmap->add_region(0x80000000, config.memory_size);
		mmio = new MMIO(mmap, config.memory_size);
		idec = new InstructionDecoder();
		idec->init_all_instrs();
	}

	void Machine::init_auto_devices()
	{
		mmio->create_device_auto<rv64vm::dev::PLIC>(*this);
		auto ptr		= mmio->create_device_auto<rv64vm::dev::UART>(*this);
		ptr->out_stream = uart_out;
		mmio->create_device_auto<rv64vm::dev::CLINT>(*this);
		mmio->create_device_auto<rv64vm::dev::SYSCON>(*this);
		mmio->create_device_auto<rv64vm::dev::I2C>(*this);
		if(image_file != nullptr)
		{
			mmio->create_device_auto<rv64vm::dev::VirtIO_BLK>(*this);
		}
	}

	// little helpers to compact the code
	inline std::vector<char> read_file(FILE* file)
	{
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		fseek(file, 0, SEEK_SET);

		std::vector<char> buffer(size + 1, '\0');
		fread(buffer.data(), 1, size, file);
		buffer[size] = '\0';

		return buffer;
	}

	void Machine::run()
	{
		if(!bios_file) return;

		auto buffer				= read_file(bios_file);
		uint64_t local_entry_pc = config.entry_pc;
		bool out				= mmap->load_buffer(0x80000000, buffer.data(), buffer.size() - 1, &local_entry_pc);
		if(!out) return;

		if(kernel_file != nullptr)
		{
			auto buffer = read_file(kernel_file);
			out			= mmap->load_buffer(0x80200000, buffer.data(), buffer.size() - 1);
		}
		if(!out) return;

		uint64_t dtb_path_in_memory = 0x80000000 + config.memory_size - 0x20000;
		// init all harts
		for(int i = 0; i < config.hart_count; i++)
		{
			Hart& h = harts[i];
			h.mmap	= mmap;
			h.mmio	= mmio;
			h.idec	= idec;

			h.init(dtb_path_in_memory, local_entry_pc);
		}

// prepare
#ifdef USE_GDBSTUB
		state = gdb ? MachineState::Halted : MachineState::Running;
		listen_gdb(gdb_port);
#else
		state = MachineState::Running;
#endif

		// create work thread
		work_thread			= std::thread(&Machine::work, this);
		work_thread_running = true;
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
#ifdef USE_GDBSTUB
				gdb_server.stop();
#endif

				if(bios_file)
				{
					auto buffer = read_file(bios_file);
					mmap->load_buffer(0x80000000, buffer.data(), buffer.size() - 1);
				}
				if(kernel_file != nullptr)
				{
					auto buffer = read_file(kernel_file);
					mmap->load_buffer(0x80200000, buffer.data(), buffer.size() - 1);
				}

				write_fdt();

				// Init harts
				uint64_t dtb_path_in_memory = 0x80000000 + config.memory_size - 0x20000;
				harts.clear();
				for(int i = 0; i < config.hart_count; i++)
				{
					harts.emplace_back(i, config.memory_size);
					Hart& hart = harts.back();
					hart.mmap  = mmap;
					hart.mmio  = mmio;
					hart.idec  = idec;

					hart.init(dtb_path_in_memory, config.entry_pc);
				}
// prepare
#ifdef USE_GDBSTUB
				state.store(gdb ? MachineState::Halted : MachineState::Running, std::memory_order_release);
				listen_gdb(gdb_port);
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
					state.store(MachineState::Running, std::memory_order_release);
				}
#endif
				std::this_thread::yield();
				continue;
			}
#ifdef USE_GDBSTUB
			handle_gdb_breakpoints();
#endif

			// Update devices
			dev_tick_time++;
			if(dev_tick_time == 0x1000)
			{
				dev_tick_time = 0;
				mmio->tick_all();
			}
			// Update harts
			for(int i = 0; i < config.hart_count; i++)
			{
				harts[i].tick();
			}

#ifdef USE_GDBSTUB
			if(gdb_single_step)
			{
				gdb_single_step = false;
				state.store(MachineState::Halted, std::memory_order_release);
			}
#endif
		}
		work_thread_running = false;
#ifdef USE_GDBSTUB
		gdb_server.stop();
#endif
	}

	void Machine::wait()
	{
		if(!work_thread_running.exchange(false, std::memory_order_acq_rel))
		{
			return;
		}
		if(std::this_thread::get_id() == work_thread.get_id())
		{
			work_thread_running.store(true, std::memory_order_release);
			return;
		}
		std::thread th_to_join;
		try
		{
			th_to_join = std::move(work_thread);
		}
		catch(...)
		{
			return;
		}

		if(th_to_join.joinable())
		{
			th_to_join.join();
		}
	}

	void Machine::stop()
	{
		state.store(MachineState::Off, std::memory_order_release);
#ifdef USE_GDBSTUB
		gdb_server.stop();
#endif
		if(!work_thread_running.exchange(false, std::memory_order_acq_rel))
		{
			return;
		}

		if(std::this_thread::get_id() != work_thread.get_id())
		{
			std::thread th_to_join;
			try
			{
				th_to_join = std::move(work_thread);
			}
			catch(...)
			{
				return;
			}

			if(th_to_join.joinable())
			{
				th_to_join.join();
			}
		}
		work_thread_running = false;
	}

	void Machine::reset()
	{
		if(state.load() == MachineState::Off)
		{
			destroy_harts();
			reset_memory();

			auto buffer = read_file(bios_file);
			mmap->load_buffer(0x80000000, buffer.data(), buffer.size() - 1);
			if(kernel_file != nullptr)
			{
				auto buffer = read_file(kernel_file);
				mmap->load_buffer(0x80200000, buffer.data(), buffer.size());
			}

			write_fdt();

			// Init harts
			uint64_t dtb_path_in_memory = 0x80000000 + config.memory_size - 0x20000;
			for(int i = 0; i < config.hart_count; i++)
			{
				harts.emplace_back(i, config.memory_size);
				Hart& hart = harts.back();
				hart.mmap  = mmap;
				hart.mmio  = mmio;
				hart.idec  = idec;

				hart.init(dtb_path_in_memory, config.entry_pc);
			}
// prepare
#ifdef USE_GDBSTUB
			state.store(gdb ? MachineState::Halted : MachineState::Running, std::memory_order_release);
			listen_gdb(gdb_port);
#else
			state.store(MachineState::Running, std::memory_order_release);
#endif
		}
		else
			state.store(MachineState::Resetting, std::memory_order_release);
	}

	void Machine::reset_memory()
	{
		if(!mmap) return;
		for(auto* reg : mmap->get_regions())
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
		if(mmio)
		{
			// If you think this will not remove actual objects from heap - it will
			mmio->devs.clear();
		}
	}
	void Machine::destroy_mmap()
	{
		if(!mmap) return;
		for(auto* reg : mmap->get_regions())
		{
			delete reg;
		}
		mmap->get_regions().clear();
		delete mmap;
		mmap = nullptr;
		delete mmio;
		mmio = nullptr;
	}
}
