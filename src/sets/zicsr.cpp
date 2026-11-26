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

#include "../../include/decode.hpp"
#include "../../include/defines/csr.hpp"
#include "../../include/hart.hpp"
#ifdef USE_JIT
#include "../../include/jit/rvjit.hpp"
#include "../../include/jit/rvjit_ctx.hpp"
#include "../../include/jit/rvjit_x86_64.hpp"
#endif

using namespace rv64vm::runner;
// ZiCSR

using PrivilegeMode = Hart::PrivilegeMode;
bool compatible_mode(PrivilegeMode csr_level, PrivilegeMode cur_mode)
{
	if(csr_level == PrivilegeMode::Machine)
	{
		return cur_mode == PrivilegeMode::Machine;
	}
	else if(csr_level == PrivilegeMode::Supervisor)
	{
		return (cur_mode == PrivilegeMode::Machine) || (cur_mode == PrivilegeMode::Supervisor);
	}
	else if(csr_level == PrivilegeMode::User)
	{
		return (cur_mode == PrivilegeMode::Machine) || (cur_mode == PrivilegeMode::Supervisor) || (cur_mode == PrivilegeMode::User);
	}
	return true;
}

bool csr_accessible(uint16_t csr_addr, PrivilegeMode current_priv, bool write)
{
	PrivilegeMode csr_level;

	if(csr_addr >= 0x000 && csr_addr <= 0x0FF)
		csr_level = PrivilegeMode::User; // User
	else if(csr_addr >= 0x100 && csr_addr <= 0x1FF)
		csr_level = PrivilegeMode::Supervisor; // Supervisor
	else if(csr_addr >= 0x200 && csr_addr <= 0x2FF)
		csr_level = PrivilegeMode::Hypervisor; // Hypervisor
	else if(csr_addr >= 0x300 && csr_addr <= 0x3FF)
		csr_level = PrivilegeMode::Machine; // Machine
	else if(csr_addr >= 0xB00 && csr_addr <= 0xBFF)
		csr_level = PrivilegeMode::Machine; // Machine counters
	else if(csr_addr >= 0xC00 && csr_addr <= 0xCFF)
		csr_level = PrivilegeMode::User; // User counters (cycle/time)
	else if(csr_addr >= 0xF00 && csr_addr <= 0xFFF)
		csr_level = PrivilegeMode::Machine; // Machine info
	else
		csr_level = PrivilegeMode::User;

	if(!compatible_mode(csr_level, current_priv))
	{
		return false;
	}

	if(csr_addr == CSR_SCOUNTOVF
	   || (csr_addr >= CSR_SSTATEEN0 && csr_addr <= CSR_SSTATEEN3)
	   || (csr_addr >= CSR_MSTATEEN0 && csr_addr <= CSR_MSTATEEN3)
	   || csr_addr == CSR_MTOPI
	   || csr_addr == CSR_TSELECT
	   || (csr_addr >= CSR_TDATA1 && csr_addr <= CSR_TDATA3)
	   || csr_addr == CSR_MCYCLECFG
	   || csr_addr == CSR_MINSTRET)
	{
		// Unimplemented
		return false;
	}

	if(csr_addr == 0xF11
	   || csr_addr == 0xF12
	   || csr_addr == 0xF13
	   || csr_addr == 0xF14
	   || csr_addr == 0xF15f
	   || (csr_addr >= 0xC00 && csr_addr <= 0xC1F))
	{
		return !write; // RO
	}

	return true;
}

bool counter_enabled(uint16_t addr, Hart& hart)
{
	if(addr < 0xC00 || addr > 0xC1F) return true;
	PrivilegeMode mode	 = hart.mode;
	uint64_t mcounteren	 = hart.csr_read(CSR_MCOUNTEREN);
	uint64_t scounteren	 = hart.csr_read(CSR_SCOUNTEREN);
	uint64_t addr_offset = 1ULL << (addr - 0xC00); // start of user counter's
	switch(mode)
	{
		case PrivilegeMode::Machine:
			return true;
		case PrivilegeMode::Supervisor:
			return (mcounteren & addr_offset) == addr_offset;
		case PrivilegeMode::User:
			return (mcounteren & addr_offset) == addr_offset && (scounteren & addr_offset) == addr_offset;
		default:
			return true;
	}
}

ExecReturn exec_CSRRW(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!counter_enabled(inst.imm, hart))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, true))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	hart.csr_write(inst.imm, hart.GPR[inst.rs1]);
	if(inst.rd != 0)
	{
		hart.GPR[inst.rd] = init_val;
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRS(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!counter_enabled(inst.imm, hart))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = hart.GPR[inst.rs1];
		hart.csr_write(inst.imm, init_val | mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRC(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!counter_enabled(inst.imm, hart))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = hart.GPR[inst.rs1];
		hart.csr_write(inst.imm, init_val & ~mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_CSRRWI(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!counter_enabled(inst.imm, hart))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, inst.rs1 != 0))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	if(inst.rd != 0)
	{
		hart.GPR[inst.rd] = hart.csr_read(inst.imm);
	}
	hart.csr_write(inst.imm, inst.rs1);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRSI(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!counter_enabled(inst.imm, hart))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = inst.rs1;
		hart.csr_write(inst.imm, init_val | mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRCI(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!counter_enabled(inst.imm, hart))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = inst.rs1;
		hart.csr_write(inst.imm, init_val & ~mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}

#ifdef USE_JIT
// Called from generated code. The exec functions above remain the single
// source of truth for access checks, masks, aliases and CSR side effects.
extern "C" uint64_t rvjit_zicsr_exec(rv64vm::jit::JIT_HartContext* ctx, uint64_t packed, uint32_t inst)
{
	using namespace rv64vm;
	const uint32_t op			  = (uint32_t)(packed & 7);
	const uint16_t csr			  = (uint16_t)((packed >> 3) & 0xFFF);
	const uint8_t rs1			  = (uint8_t)((packed >> 15) & 0x1F);
	const uint8_t rd			  = (uint8_t)((packed >> 20) & 0x1F);
	const uint32_t instr_index	  = (uint32_t)((packed >> 25) & 0xFFFF);
	const uint32_t instr_bytes	  = (uint32_t)((packed >> 41) & 0xFFFF);

	Hart& hart = *ctx->hart;
	InstructionData d;
	d.inst = inst;
	d.imm  = csr;
	d.rs1  = rs1;
	d.rs2  = 0;
	d.rd   = rd;
	d.size = 4;

	// The JIT runner credits cycle/instret only when a chain returns to C++.
	// Compensate while exec reads them, then undo: this block's instructions
	// are still credited by the runner on return.
	const uint64_t done = (uint64_t)jit::x86::CHAIN_CADENCE - (uint64_t)ctx->chain_budget;
	hart.cycle += done;
	hart.instret += done;

	// Generated code does not keep Hart::pc in sync per instruction; trap()
	// needs the precise faulting pc (and writes mepc from it).
	hart.pc = ctx->entry_pc + instr_bytes;
	const uint64_t pc_before = hart.pc;
	ExecReturn r;
	switch(op)
	{
		case 0: r = exec_CSRRW(hart, d); break;
		case 1: r = exec_CSRRS(hart, d); break;
		case 2: r = exec_CSRRC(hart, d); break;
		case 3: r = exec_CSRRWI(hart, d); break;
		case 4: r = exec_CSRRSI(hart, d); break;
		case 5: r = exec_CSRRCI(hart, d); break;
		default: r = exec_CSRRW(hart, d); break;
	}

	hart.cycle -= done;
	hart.instret -= done;
	hart.GPR[0] = 0; // CSRRS/CSRRC write rd unconditionally, including x0

	if(!r.is_success)
	{
		hart.trap(r.cause, r.tval, false);
		ctx->exit_pc	   = hart.pc;
		ctx->chain_budget -= (int64_t)instr_index;
		return 1;
	}

	// csr_read/csr_write can trap internally (FP CSRs with FS == 0). The
	// interpreter still retires the instruction and adds 4 to pc afterwards.
	if(hart.pc != pc_before)
	{
		ctx->exit_pc	   = hart.pc + 4;
		ctx->chain_budget -= (int64_t)instr_index + 1;
		return 1;
	}

	// A write to these CSRs changes state baked into compiled blocks (effective
	// mode / permission masks / TLB generation), so the chain must stop.
	const bool writes = (op == 0 || op == 3 || rs1 != 0);
	if(writes && (csr == CSR_SSTATUS || csr == CSR_MSTATUS || csr == CSR_SATP))
	{
		ctx->exit_pc	   = ctx->entry_pc + instr_bytes + 4;
		ctx->chain_budget -= (int64_t)instr_index + 1;
		return 1;
	}

	return 0;
}
#endif

void InstructionDecoder::init_zicsr()
{
	auto inst_CSRRW = register_instr("*****************001*****1110011", exec_CSRRW, imm_Zicsr);
	auto inst_CSRRS = register_instr("*****************010*****1110011", exec_CSRRS, imm_Zicsr);
	auto inst_CSRRC = register_instr("*****************011*****1110011", exec_CSRRC, imm_Zicsr);
	auto inst_CSRRWI = register_instr("*****************101*****1110011", exec_CSRRWI, imm_Zicsr);
	auto inst_CSRRSI = register_instr("*****************110*****1110011", exec_CSRRSI, imm_Zicsr);
	auto inst_CSRRCI = register_instr("*****************111*****1110011", exec_CSRRCI, imm_Zicsr);
#ifdef USE_JIT
	inst_CSRRW->jit_func  = &jit::jit_CSRRW;
	inst_CSRRS->jit_func  = &jit::jit_CSRRS;
	inst_CSRRC->jit_func  = &jit::jit_CSRRC;
	inst_CSRRWI->jit_func = &jit::jit_CSRRWI;
	inst_CSRRSI->jit_func = &jit::jit_CSRRSI;
	inst_CSRRCI->jit_func = &jit::jit_CSRRCI;
#endif
}
