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
#include "libfdt.h"
#include "memory_map.hpp"
#include "mmio.hpp"
#include <thread>
#include <vector>

namespace rv64vm::runner
{
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

	/**
	 * @ingroup RV64VM-API
	 * @brief RV64-VM Main machine class.
	 * @details This class implements RISC-V emulator machine.
	 */
	class Machine
	{
	  public:
		/**
		 * @brief Machine constructor
		 * @details Creates RISC-V machine
		 * @param cfg Machine configuration
		 */
		Machine(const MachineConfig& cfg);
		/**
		 * @brief Machine destructor
		 * @details Destroys RISC-V machine
		 * @note It is recommended to stop machine before destroy it.
		 */
		~Machine();

		// Disable copy/move mechanics to keep the internal thread safe
		Machine(const Machine&)			   = delete;
		Machine& operator=(const Machine&) = delete;

		/**
		 * @brief Device initialization start
		 * @details Creates FDT Base for all devices
		 *			In this block you supposed to create all devices you want, or load DTB from file.
		 */
		void start_init()
		{
			if(config.init_fdt) init_fdt();
			init_auto_devices(); // Inits all SoC
		}
		/**
		 * @brief Device initialization end
		 * @details Writes FDT to memory.
		 *			This function must be called after you created all devices you wanted.
		 */
		void end_init()
		{
			write_fdt();
		}

		/**
		 * @brief Runs machine
		 * @details Starts all Hart's execution loop.
		 */
		void run();
		/**
		 * @brief Stops machine
		 * @details Sends a signal to machine so it could stop and destroy all harts safely.
		 * @note In that moment it joins work thread.
		 */
		void stop();
		/**
		 * @brief Resets machines
		 * @details Sends a signal to machine so it could safely recreate all HART's.
		 * @warning Untested, but it is not recommended to first-time start machine with this function.
		 */
		void reset();
		/**
		 * @brief Joins machine work thread
		 */
		void wait();

		/**
		 * @brief Returns MMIO pointer
		 * @see MMIO
		 * @return MMIO Pointer
		 */
		MMIO* get_mmio() { return mmio; }
		/**
		 * @brief Returns FDT pointer
		 * @see libfdt.h
		 * @return FDT pointer
		 */
		fdt_node* get_fdt() { return fdt; }
		/**
		 * @brief Returns config specified timer timebase (Hz/S)
		 * @return Timebase number
		 */
		uint64_t get_timebase() const { return config.timebase; }
		/**
		 * @brief Returns MemoryMap pointer
		 * @see MemoryMap
		 * @return MemoryMap pointer
		 */
		MemoryMap* get_mmap() { return mmap; }

		/**
		 * @brief Returns Machine internal state
		 * @see MachineState
		 * @return Machine State enum
		 */
		MachineState get_state() const { return state.load(); }
		/**
		 * @brief Returns config specified RAM size
		 * @return Memory Size (bytes)
		 */
		uint64_t get_memory_size() const { return config.memory_size; }
		/**
		 * @brief Returns config specified Hart count
		 * @see HART
		 * @return HART count
		 */
		uint8_t get_hart_count() const { return config.hart_count; }
		/**
		 * @brief Returns specified Hart by index
		 * @see Hart
		 * @param index HART index (not ID!)
		 * @return Hart object
		 */
		Hart& get_hart(size_t index) { return harts.at(index); }

		/**
		 * @brief Loads Image file
		 * @param path Image path
		 * @return Success bool
		 */
		bool load_image(const std::string& path);
		/**
		 * @brief Loads Firmware file
		 * @param path Firmware path
		 * @return Success bool
		 */
		bool load_bios(const std::string& path);
		/**
		 * @brief Loads Kernel file
		 * @param path Kernel path
		 * @return Success bool
		 */
		bool load_kernel(const std::string& path);
		/**
		 * @brief Loads DTB file
		 * @details Loads custom FDT from DTB to memory.
		 * @note Make sure to set init_fdt in config to false before init!
		 * @param path DTB path
		 * @return Success bool
		 */
		bool load_dtb(const std::string& path);
		/**
		 * @brief Returns FILE pointer to loaded Image file
		 * @return FILE pointer
		 */
		FILE* get_image();
		/**
		 * @brief Sets UART output stream
		 * @param stream Output stream
		 */
		void set_uart_output(FILE* stream);
		/**
		 * @brief Returns UART output stream
		 * @return Output stream
		 */
		FILE* get_uart_output();

#ifdef USE_GDBSTUB
		/**
		 * @brief Enables GDB in machine.
		 * @param enable State
		 * @param port GDB Server Port
		 */
		void enable_gdb(bool enable, uint16_t port);
		/**
		 * @brief Sets GDB single step var
		 * @internal
		 * @param single_step State
		 */
		void set_gdb_single_step(bool single_step);

		/**
		 * @brief Starts GDB server
		 * @internal
		 * @param port GDB Server Port
		 */
		void listen_gdb(uint16_t port);
		/**
		 * @brief Tick checker function for all set breakpoints.
		 * @internal
		 */
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
			~GDBStub()
			{
				if(worker_thread.joinable())
				{
					worker_thread.detach();
				}
			}

			void start(uint16_t port);
			void stop();

			uint32_t send_packet(std::string buffer, uint32_t flags = 0);

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

			std::atomic<bool> is_executing{ true };
			std::atomic<bool> received_sigint{ false };
			std::vector<uint64_t> breakpoints;

			friend class Machine;
		};
		GDBStub gdb_server{ this };
#endif
	};
}
