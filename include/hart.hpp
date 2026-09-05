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

#include "block_cache.hpp"
#include "decode.hpp"
#include "defines/csr.hpp"
#include "defines/traps.hpp"
#include "memory_map.hpp"
#include "mmio.hpp"
#include "mmu.hpp"
#include "structs/timecmp_st.hpp"
#ifdef USE_JIT
#include "jit/rvjit_ctx.hpp"
namespace rv64vm::jit
{
	class JIT_Context;
}
#endif
#include <cstdint>
#include <sys/types.h>
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
			uint64_t paddr;
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
		~Hart() {
		};
		Hart(Hart&& other) noexcept
		{
		}

		Hart& operator=(Hart&& other) noexcept
		{
			if(this != &other)
			{
			}
			return *this;
		}
		uint8_t id;
		uint64_t GPR[32];
		/*class GPR
		{
		  public:
			uint64_t& operator[](size_t index)
			{
				if(index == 0)
				{
					zero_dummy = 0;
					return zero_dummy;
				}
				return GPR[index];
			}
			const uint64_t& operator[](size_t index) const
			{
				return GPR[index];
			}

		  private:
			std::array<uint64_t, 32> GPR;
			uint64_t zero_dummy;
		};
		GPR GPR;*/
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
		satp_t satp;
		uint64_t cycle;
		uint64_t instret;
		uint64_t ctime;
		bool WFI = false;
#ifdef USE_JIT
		// Native JIT state (set up by Machine; used by run_blocks).
		jit::JIT_HartContext hctx{};
		jit::JIT_Context* jctx = nullptr;
#endif

		/**
		 * @brief Returns CPU Effective mode for a specific memory access
		 * @details Effective mode corresponds to MPRV bit in mstatus:
		 *          1 - MPP (only for LOAD/STORE), 0 - current mode
		 * @return Effective mode
		 * @see PrivilegeMode
		 */
		inline PrivilegeMode get_effective_mode(AccessType access_type) const
		{
			if(mode == PrivilegeMode::Machine && status.fields.MPRV && (access_type == AccessType::LOAD || access_type == AccessType::STORE))
			{
				return static_cast<PrivilegeMode>(status.fields.MPP);
			}
			return mode;
		}
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
		 * @brief Returns CPU Memory Management Unit
		 * @see MMU
		 * @return MMU reference
		 */
		inline MMU& get_mmu() { return mmu; }
		/**
		 * @brief Clears Instruction Decoder Cache
		 */
		inline void clear_decode_cache()
		{
			/*for(int i = 0; i < CACHE_SIZE; i++)
			{
				idec->cache[i].ways[0].pc = 0;
				idec->cache[i].ways[1].pc = 0;
				idec->cache[i].victim	  = 0;
			}*/
			idec->cache_generation++;
		}
		/**
		 * @brief Returns RAM size
		 * @return Memory size in bytes
		 */
		const inline uint64_t get_memsize() const { return memsize; }
		/**
		 * @brief Clears reservation if defined address is within CPU reservation address
		 * @param va Virtual Address
		 */
		inline void amo_check_reservation(uint64_t pa)
		{
			if(reservation.valid && reservation.paddr >= pa && pa <= reservation.paddr + (int)reservation.size)
			{
				reservation.valid = false;
			}
		}
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

		InstructionDecoder* idec;
		MemoryMap* mmap;
		MMIO* mmio;
		MMU mmu;

		uint64_t memsize	= 0;
		uint8_t* direct_ram = nullptr;
		Reservation reservation;

		void init(uint64_t dtb_pos_at_memory, uint64_t entry_pc);
		MemoryReturn fetchInstruction(uint64_t va, uint64_t& phys_pc, rv64vm::runner::InstructionCache*& out_cache);
		void tick();
		ExecReturn single_inst(InstructionCache& cache);
		bool int_local_pending();
		bool check_ints();

		/**
		 * @brief Block-chaining fast path (lightweight JIT).
		 * @details Runs precompiled straight-line blocks of the guest code,
		 *          bypassing the per-instruction decode probe and MMU
		 *          translate. Returns the number of instructions executed.
		 * @see BlockCache
		 */
		uint64_t run_blocks(BlockCache& bc, uint64_t max_insts);
		Block* compile_block(BlockCache& bc, uint64_t start_phys);

		friend class Machine;
#ifdef USE_JIT
		friend class jit::JIT_Context;
#endif
	};
}

#include "mmu.inl"
