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

#include "decode.hpp"
#include "defines/csr.hpp"
#include "defines/traps.hpp"
#include "memory_map.hpp"
#include "mmio.hpp"
#include "rvjit/rvjit.hpp"
#include "structs/timecmp_st.hpp"
#include <cstdint>
namespace rv64vm::runner
{
	class MMIO;

	struct InstructionCache;

	/**
	 * @ingroup RV64VM-API
	 * @brief RISC-V CPU Core
	 */
	class Hart
	{
	  public:
		/**
		 * @brief CPU PrivilegeMode
		 * @details Current CPU privilege mode.
		 */
		enum class PrivilegeMode
		{
			User	   = 0,
			Supervisor = 1,
			Hypervisor = 2,
			Machine	   = 3
		};
		/**
		 * @brief CPU Atomic Reservation
		 * @details Used by RV64 Atomic Extension
		 */
		struct Reservation
		{
			uint64_t vaddr;
			MemorySize size;
			bool valid;
		};

		/**
		 * @brief Hart constructor
		 * @details Creates RISC-V core
		 * @param id Internal Hart ID (starts from 0)
		 * @param memsize Memory size
		 * @note It must be equal with memory size defined in machine config.
		 */
		Hart(uint8_t id, uint64_t memsize);
		Hart(const Hart&)			 = delete;
		Hart& operator=(const Hart&) = delete;
		/**
		 * @brief Hart destructor
		 * @details Destroys RISC-V core
		 */
		~Hart()
		{
#ifdef USE_JIT
			delete jctx;
#endif
		};
		Hart(Hart&& other) noexcept
		{
#ifdef USE_JIT
			jctx	   = other.jctx;
			other.jctx = nullptr;
#endif
		}

		Hart& operator=(Hart&& other) noexcept
		{
			if(this != &other)
			{
#ifdef USE_JIT
				delete jctx;
				jctx	   = other.jctx;
				other.jctx = nullptr;
#endif
			}
			return *this;
		}
		uint8_t id;
		uint64_t GPR[32];
#ifdef USE_FPU
		double FPR[32];
#endif
		uint64_t pc;
		PrivilegeMode mode;
		status_t status;
		ie_t ie;
		ip_t ip;
		timecmp_st stimecmp;
		fcsr_t fcsr;
		bool WFI = false;

		/**
		 * @brief Returns MMIO pointer
		 * @see MMIO
		 * @return MMIO Pointer
		 */
		inline MMIO* get_mmio() { return mmio; }
		/**
		 * @brief Returns MemoryMap pointer
		 * @see MemoryMap
		 * @return MemoryMap pointer
		 */
		inline MemoryMap* get_mmap() { return mmap; }
		/**
		 * @brief Returns CPU Atomic Reservation
		 * @see Reservation
		 * @return Reservation reference object
		 */
		inline Reservation& get_reservation() { return reservation; }
		/**
		 * @brief Clears Instruction Decoder Cache
		 */
		inline void clear_decode_cache()
		{
			for(int i = 0; i < CACHE_SIZE; i++)
			{
				idec->cache[i].ways[0].valid = false;
				idec->cache[i].ways[1].valid = false;
				idec->cache[i].victim		 = 0;
			}
		}
		/**
		 * @brief Clears reservation if defined address is within CPU reservation address
		 * @param va Virtual Address
		 */
		inline void amo_check_reservation(uint64_t va)
		{
			if(reservation.valid && reservation.vaddr >= va && va <= reservation.vaddr + (int)reservation.size)
			{
				reservation.valid = false;
			}
		}
#ifdef USE_JIT
		/**
		 * @brief Returns JIT context
		 * @return JIT context
		 */
		inline jit::JIT_Context* get_jctx() { return jctx; }
#endif
		/**
		 * @brief Returns value stored in CSR
		 * @param csr CSR address
		 * @return CSR value
		 */
		uint64_t csr_read(uint16_t csr);
		/**
		 * @brief Stores value to CSR
		 * @param csr CSR address
		 * @param val Value
		 */
		void csr_write(uint16_t csr, uint64_t val);

		/**
		 * @brief CPU Trap function
		 * @details Raises trap in CPU core.
		 * @param cause Trap cause
		 * @param tval Trap value (can be zero)
		 * @param interrupt Is trap will be interrupt(true) of exception(false)?
		 */
		void trap(uint64_t cause, uint64_t tval, bool interrupt);

	  private:
		uint64_t csrs[4096];

#ifdef USE_JIT
		jit::JIT_Context* jctx;
		jit::JIT_HartContext hctx;
		uint64_t last_jit_pc_exit = 0;
#endif
		InstructionDecoder* idec;
		MemoryMap* mmap;
		MMIO* mmio;

		Reservation reservation;

		void init(uint64_t dtb_pos_at_memory, uint64_t entry_pc);
		void tick();
		ExecReturn single_inst(InstructionCache& cache);
		uint32_t fetch(uint64_t inst_pc);
		bool int_local_pending();
		bool check_ints();

		friend class Machine;
	};
}
