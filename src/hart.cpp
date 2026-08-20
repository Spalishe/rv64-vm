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

#include "../include/hart.hpp"
#include "../include/decode.hpp"
#include "../include/defines/csr.hpp"
#include "../include/defines/traps.hpp"
#include <assert.h>
#include <cstdio>

namespace rv64vm::runner
{
	Hart::Hart(uint8_t id, uint64_t memsize) : id(id), memsize(memsize)
	{
#ifdef USE_JIT
		jctx	  = new jit::JIT_Context(memsize);
		hctx	  = jit::JIT_HartContext();
		hctx.hart = this;
#endif
	};
	void Hart::init(uint64_t dtb_pos_at_memory, uint64_t entry_pc)
	{
		pc				  = entry_pc;
		mode			  = PrivilegeMode::Machine;
		GPR[10]			  = id;
		GPR[11]			  = dtb_pos_at_memory;
		csrs[CSR_MISA]	  = (1ULL << 63) | (1ULL << 8) | (1ULL << 12) | (1ULL << 0) | (1ULL << 5) | (1ULL << 3) | (1ULL << 2) | (1ULL << 1) | (1ULL << 18) | (1ULL << 20); // RV64IMAFDCBSU
		csrs[CSR_MHARTID] = id;
		status.fields.SXL = 2;
		status.fields.UXL = 2;
		mmu.mmap		  = mmap;
#ifdef USE_JIT
		hctx.regs	 = GPR;
		hctx.mmio	 = mmio;
		hctx.ram	 = mmap->get_ram_direct()->ptr(0x80000000);
		hctx.memsize = mmap->get_ram_direct()->get_size();
#endif
	}

	ExecReturn Hart::single_inst(InstructionCache& cache)
	{
		ExecReturn out = cache.inst->func(*this, cache.data);
		return out;
	}

	void Hart::tick()
	{
		GPR[0] = 0;
		cycle++;
		if((ip.raw & ie.raw) != 0) [[unlikely]]
			check_ints();
		if(WFI) [[unlikely]]
		{
			// We must continue execution even if we has locally pending interruptions
			if(int_local_pending()) WFI = false;

			return;
		}

		uint64_t prevpc = pc;
#ifdef USE_JIT
		if(jctx->count != 0)
		{
			jit::JIT_Function& jit_entry = jctx->jits[jit::jit_index(pc)];

			if(jit_entry.valid && jit_entry.pc == pc) [[likely]]
			{
				if(jit_entry.page_version != jctx->page_verion_bitmap[(pc - 0x80000000) >> 12]) [[unlikely]]
				{
					jit_entry.cleanup(jctx);
					return;
				}

				hctx.exit_pc	= 0;
				hctx.loop_count = 1000;

				jit_entry.func(&hctx);

				if(hctx.exit_pc != 0)
				{
					pc = hctx.exit_pc;
				}
				else
				{
					pc += jit_entry.inst_size;
				}

				return;
			}
		}
#endif
		// MemoryReturn out1 = mmio->read(*this, pc, MemorySize::Int, &inst);
		uint64_t phys_pc = 0;

		MemoryReturn ret = mmu.translate(this, AccessType::EXEC, pc, &phys_pc);

		if(!ret.is_success)
		{
			trap(ret.exc_code, ret.tval, false);
			return;
		}

		if(direct_ram == nullptr)
			direct_ram = mmap->get_ram_direct()->get_data();

		if(phys_pc < 0x80000000ULL || phys_pc > 0x80000000ULL + memsize - 2) [[unlikely]]
		{
			trap(EXC_INST_ACCESS_FAULT, pc, false);
			return;
		}

		uint16_t lo = *(uint16_t*)(direct_ram + (phys_pc - 0x80000000ULL));
		uint32_t inst;

		if((lo & 0x3) != 0x3)
		{
			inst = lo;
		}
		else if((pc & 0xFFFULL) <= 0xFFCULL) [[likely]]
		{
			// Full 4-byte instruction
			inst = *(uint32_t*)(direct_ram + (phys_pc - 0x80000000ULL));
		}
		else
		{
			uint64_t phys_pc2 = 0;
			MemoryReturn ret2 = mmu.translate(this, AccessType::EXEC, pc + 2, &phys_pc2);
			if(!ret2.is_success)
			{
				trap(ret2.exc_code, ret2.tval, false);
				return;
			}
			if(phys_pc2 < 0x80000000ULL || phys_pc2 > 0x80000000ULL + memsize - 2)
			{
				trap(EXC_INST_ACCESS_FAULT, pc + 2, false);
				return;
			}
			uint16_t hi = *(uint16_t*)(direct_ram + (phys_pc2 - 0x80000000ULL));
			inst		= (uint32_t)lo | ((uint32_t)hi << 16);
		}
		// if(!out1.is_success) [[unlikely]]
		//	trap(EXC_INST_ACCESS_FAULT, out1.tval, false);

		//  uint32_t inst = fetch(pc);

		InstructionCache& cache = idec->decode_inst(phys_pc, inst);
		if(cache.pc == 0)
		{
#ifdef USE_JIT
			jctx->stopBlock();
#endif
			trap(EXC_ILLEGAL_INSTRUCTION, inst, false);
			return;
		}

		// Run single instruction
		auto out = single_inst(cache);
		if(!out.is_success)
		{
#ifdef USE_JIT
			jctx->stopBlock();
#endif
			trap(out.cause, out.tval, false);
			return;
		}
		else
		{
			instret++;
			pc += out.increase_pc;
		}
#ifdef USE_JIT
		jctx->handleInstruction(*this, cache, prevpc);
#endif
	}

	bool Hart::int_local_pending()
	{
		if((ip.raw & ie.raw) == 0)
			return false;
		uint64_t sip	 = ip.raw & SE_MASK;
		uint64_t sie	 = ie.raw & SE_MASK;
		uint64_t mideleg = csrs[CSR_MIDELEG];

		uint64_t pending   = ip.raw & ie.raw & ~mideleg;
		uint64_t pending_s = sip & sie & mideleg;

		if(mode > PrivilegeMode::Supervisor) pending_s = 0;

		return pending | pending_s;
	}

	bool Hart::check_ints()
	{
		uint64_t pending_all = ip.raw & ie.raw;

		if(!pending_all)
			return false;
		uint64_t sip	 = ip.raw & SE_MASK;
		uint64_t sie	 = ie.raw & SE_MASK;
		uint64_t mideleg = csrs[CSR_MIDELEG];

		bool m_global = (mode == PrivilegeMode::Machine)
							? status.fields.MIE
							: (mode < PrivilegeMode::Machine);

		bool s_global = (mode == PrivilegeMode::Supervisor)
							? status.fields.SIE
							: (mode < PrivilegeMode::Supervisor);

		if(m_global)
		{
			uint64_t pending = ip.raw & ie.raw & ~mideleg;

			if(pending)
			{
				for(int irq : irq_priority)
				{
					if(pending & (1ULL << irq))
					{
						trap(irq, 0, true);
						return true;
					}
				}
			}
		}
		if(s_global)
		{
			uint64_t pending = (sip & mideleg) & sie;
			if(pending)
			{
				for(int irq : irq_priority)
				{
					if(pending & (1ULL << irq))
					{
						trap(irq, 0, true);
						return true;
					}
				}
			}
		}

		return false;
	}

	void Hart::trap(uint64_t cause, uint64_t tval, bool interrupt)
	{
		reservation.valid		= false;
		WFI						= false;
		uint64_t trap_pc		= pc;
		PrivilegeMode prev_mode = mode;

		uint64_t medeleg   = csrs[CSR_MEDELEG];
		uint64_t mideleg   = csrs[CSR_MIDELEG];
		bool delegate_to_s = false;
		if(prev_mode != PrivilegeMode::Machine)
		{
			if(interrupt)
				delegate_to_s = (mideleg >> cause) & 1ULL;
			else
				delegate_to_s = (medeleg >> cause) & 1ULL;
		}

		status.fields.MPRV = 0;
		if(delegate_to_s)
		{
			// Supervisor
			mode			   = PrivilegeMode::Supervisor;
			uint64_t vector	   = (((csrs[CSR_STVEC] & 3ULL) == 1 && interrupt) ? 4 * cause : 0);
			pc				   = (csrs[CSR_STVEC] & ~3ULL) + vector;
			csrs[CSR_SEPC]	   = trap_pc;
			csrs[CSR_SCAUSE]   = ((interrupt ? (1ULL << 63) : 0) | cause);
			csrs[CSR_STVAL]	   = tval;
			status.fields.SPIE = status.fields.SIE;
			status.fields.SIE  = 0;
			status.fields.SPP  = (prev_mode != PrivilegeMode::User);
		}
		else
		{
			// Machine
			mode			   = PrivilegeMode::Machine;
			uint64_t vector	   = (((csrs[CSR_MTVEC] & 3ULL) == 1 && interrupt) ? 4 * cause : 0);
			pc				   = (csrs[CSR_MTVEC] & ~3ULL) + vector;
			csrs[CSR_MEPC]	   = trap_pc;
			csrs[CSR_MCAUSE]   = ((interrupt ? (1ULL << 63) : 0) | cause);
			csrs[CSR_MTVAL]	   = tval;
			status.fields.MPIE = status.fields.MIE;
			status.fields.MIE  = 0;
			status.fields.MPP  = (uint8_t)prev_mode;
		}
	}

	void Hart::csr_write(uint16_t csr, uint64_t val)
	{
		switch(csr)
		{
			case CSR_MSTATUS:
				status.raw = (status.raw & MSTATUS_RO_MASK) | (val & ~MSTATUS_RO_MASK);
				break;
			case CSR_SSTATUS:
				status.raw = (status.raw & ~SSTATUS_MASK) | (val & SSTATUS_MASK);
				break;
			case CSR_SIE:
				ie.raw = (ie.raw & ~SE_MASK) | (val & SE_MASK);
				break;
			case CSR_SIP:
				ip.raw = (ip.raw & ~SE_MASK) | (val & SE_MASK);
				break;
			case CSR_MIE:
				ie.raw = val;
				break;
			case CSR_MIP:
				ip.raw = val;
				break;
			case CSR_STIMECMP:
				timecmp_set(&stimecmp, val);
				break;
			case CSR_SATP:
			{
				uint64_t old_val = satp.raw;
				satp.raw		 = val;
				if((MMU::SatpMode)satp.fields.mode == MMU::SatpMode::Sv48 || (MMU::SatpMode)satp.fields.mode == MMU::SatpMode::Sv57) satp.raw = old_val;
				break;
			}
			case CSR_FCSR:
			{
				if(!status.fields.FS)
				{
					trap(EXC_ILLEGAL_INSTRUCTION, csr, false);
					break;
				}
				status.fields.FS = 3;
				fcsr.raw		 = val;
				break;
			}
			case CSR_FFLAGS:
			{
				if(!status.fields.FS)
				{
					trap(EXC_ILLEGAL_INSTRUCTION, csr, false);
					break;
				}
				fcsr.fields.fflags = val;
				break;
			}
			case CSR_FRM:
			{
				if(!status.fields.FS)
				{
					trap(EXC_ILLEGAL_INSTRUCTION, csr, false);
					break;
				}
				fcsr.fields.frm = val;
				break;
			}

			case CSR_MVENDORID:
			case CSR_MARCHID:
			case CSR_MIMPID:
				// Read-only
				(void)csr;
				(void)val;
				break;
			default:
				csrs[csr] = val;
		}
	}
	uint64_t Hart::csr_read(uint16_t csr)
	{
		switch(csr)
		{
			case CSR_MSTATUS:
				return status.raw;
			case CSR_SSTATUS:
				return status.raw & SSTATUS_MASK;
			case CSR_SIE:
				return ie.raw & SE_MASK;
			case CSR_SIP:
				return ip.raw & SE_MASK;
			case CSR_MIP:
				return ip.raw;
			case CSR_MIE:
				return ie.raw;
			case CSR_CYCLE:
			case CSR_MCYCLE:
				return cycle;
			case CSR_INSTRET:
			case CSR_MINSTRET:
				return instret;
			case CSR_HPMCOUNTER3 ... CSR_HPMCOUNTER31:
				return csrs[csr - 0x100]; // get M versions
			case CSR_STIMECMP:
				return timecmp_get(&stimecmp);
			case CSR_SATP:
				return satp.raw;
			case CSR_FCSR:
			{
				if(!status.fields.FS)
				{
					trap(EXC_ILLEGAL_INSTRUCTION, csr, false);
					return 0;
				}
				return fcsr.raw;
			}
			case CSR_FFLAGS:
			{
				if(!status.fields.FS)
				{
					trap(EXC_ILLEGAL_INSTRUCTION, csr, false);
					return 0;
				}
				return fcsr.fields.fflags;
			}
			case CSR_FRM:
			{
				if(!status.fields.FS)
				{
					trap(EXC_ILLEGAL_INSTRUCTION, csr, false);
					return 0;
				}
				return fcsr.fields.frm;
			}
			default:
				return csrs[csr];
		}
	}
}
