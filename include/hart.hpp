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

#include "decode.hpp"
#include "defines/csr.hpp"
#include "defines/traps.hpp"
#include "memory_map.hpp"
#include "mmio.hpp"
#include "rvjit/rvjit.hpp"
#include "structs/timecmp_st.hpp"
#include <cstdint>

enum class PrivilegeMode
{
	User	   = 0,
	Supervisor = 1,
	Hypervisor = 2,
	Machine	   = 3
};

struct MMIO;

struct InstructionCache;

struct Reservation
{
	uint64_t vaddr;
	MemorySize size;
	bool valid;
};

class Hart
{
  public:
	Hart(uint8_t id, uint64_t memsize) : id(id)
	{
#ifdef USE_JIT
		jctx	  = new JIT_Context(memsize);
		hctx	  = JIT_HartContext();
		hctx.hart = this;
#endif
	};
	Hart(const Hart&)			 = delete;
	Hart& operator=(const Hart&) = delete;
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

	inline MMIO* get_mmio() { return mmio; }
	inline MemoryMap* get_mmap() { return mmap; }
	inline Reservation& get_reservation() { return reservation; }
	inline void clear_decode_cache()
	{
		for(int i = 0; i < CACHE_SIZE; i++)
		{
			idec->cache[i].ways[0].valid = false;
			idec->cache[i].ways[1].valid = false;
			idec->cache[i].victim		 = 0;
		}
	}
	inline void amo_check_reservation(uint64_t va)
	{
		if(reservation.valid && reservation.vaddr >= va && va <= reservation.vaddr + (int)reservation.size)
		{
			reservation.valid = false;
		}
	}
	inline JIT_Context* get_jctx() { return jctx; }

	uint64_t csr_read(uint16_t csr);
	void csr_write(uint16_t csr, uint64_t val);

	void trap(uint64_t cause, uint64_t tval, bool interrupt);

  private:
	uint64_t csrs[4096];

#ifdef USE_JIT
	JIT_Context* jctx;
	JIT_HartContext hctx;
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
