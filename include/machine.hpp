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
#include "device.hpp"
#include "hart.hpp"
#include "libfdt.hpp"
#include "memory_map.hpp"
#include "mmio.hpp"
#include <thread>
#include <vector>

enum class MachineState : uint8_t
{
	Off		  = 0,
	Halted	  = 1,
	Running	  = 2,
	Resetting = 3,
};

struct MachineConfig
{
	uint64_t memory_size;
	uint8_t hart_count;
	uint64_t entry_pc = 0x80000000;
	uint64_t timebase = 5'000'000ULL;
	std::string append;
	std::string dtb_dump_path;
	bool init_fdt = true;
};

class Machine
{
  public:
	Machine(const MachineConfig& cfg);
	~Machine();

	// Disable copy/move mechanics to keep the internal thread safe
	Machine(const Machine&)			   = delete;
	Machine& operator=(const Machine&) = delete;

	void start_init()
	{
		if(config.init_fdt) init_fdt();
		init_auto_devices(); // Inits all SoC
	}
	void end_init()
	{
		write_fdt();
	}

	void run();
	void stop();
	void reset();
	void wait(); // joins machine thread

	MMIO* get_mmio() { return mmio; }
	fdt_node* get_fdt() { return fdt; }
	uint64_t get_timebase() const { return config.timebase; }
	MemoryMap* get_mmap() { return mmap; }

	MachineState get_state() const { return state.load(); }
	uint64_t get_memory_size() const { return config.memory_size; }
	uint8_t get_hart_count() const { return config.hart_count; }
	Hart& get_hart(size_t index) { return harts.at(index); }

	bool load_image(const std::string& path);
	bool load_bios(const std::string& path);
	bool load_kernel(const std::string& path);
	bool load_dtb(const std::string& path);
	FILE* get_image();
	void set_uart_output(FILE* stream);
	FILE* get_uart_output();

#ifdef USE_GDBSTUB
	void enable_gdb(bool enable, uint16_t port);
	void set_gdb_single_step(bool single_step);

	void listen_gdb(uint16_t port);
	void handle_gdb_breakpoints();
#endif

  private:
	void init_mmap();
	void init_fdt();
	void write_fdt();
	void load_fdt();
	void init_auto_devices();
	void destroy_harts();
	void destroy_devices();
	void destroy_mmap();
	void reset_memory();
	void work();

	MachineConfig config;
	std::atomic<MachineState> state{ MachineState::Off };

	MemoryMap* mmap			 = nullptr;
	MMIO* mmio				 = nullptr;
	InstructionDecoder* idec = nullptr;
	fdt_node* fdt			 = nullptr;

	std::vector<Hart> harts;
	uint16_t dev_tick_time = 0;

	FILE* image_file  = nullptr;
	FILE* bios_file	  = nullptr;
	FILE* kernel_file = nullptr;
	FILE* dtb_file	  = nullptr;
	FILE* uart_out	  = stdout;

	std::thread work_thread;
	std::atomic<bool> work_thread_running = false;

#ifdef USE_GDBSTUB
	bool gdb			 = false;
	bool gdb_single_step = false;
	uint16_t gdb_port	 = 1512;

	class GDBStub
	{
	  public:
		GDBStub(Machine* parent) : machine_ctx(parent) {}
		~GDBStub() { stop(); }

		void start(uint16_t port);
		void stop();

		uint32_t send_packet(std::string buffer, uint32_t flags = 0);
		std::atomic<bool> is_executing{ true };
		std::atomic<bool> received_sigint{ false };
		std::vector<uint64_t> breakpoints;

	  private:
		void execution_loop();

		Machine* machine_ctx;
		Hart* active_hart = nullptr;

		uint32_t server_fd = 0;
		uint32_t client_fd = 0;

		std::thread worker_thread;
		std::string xml_target_description;

		std::string create_xml();
		uint32_t send_raw(const std::string& buffer, uint32_t flags);
		std::string unformat_packet(const std::string& buffer);
		void parse_packet(const std::string& buffer);
	};
	GDBStub gdb_server{ this };
#endif
};
