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
#include "../../include/hart.hpp"

// SYSTEM

ExecReturn exec_MRET(Hart& hart, InstructionData& inst)
{
	hart.pc = hart.csr_read(CSR_MEPC);
	switch(hart.status.fields.MPP)
	{
		case 0b00:
			// If MPP != M-mode, MRET also sets MPRV=0.
			hart.status.fields.MPRV = 0;
			hart.mode				= PrivilegeMode::User;
			break;
		case 0b01:
			hart.status.fields.MPRV = 0;
			hart.mode				= PrivilegeMode::Supervisor;
			break;
		case 0b11:
			hart.mode = PrivilegeMode::Machine;
			break;
	}
	// Read a previous interrupt-enable bit for machine mode (MPIE, 7), and set a global interrupt-enable bit for machine mode (MIE, 3) to it.
	hart.status.fields.MIE	= hart.status.fields.MPIE;
	hart.status.fields.MPIE = 1;
	hart.status.fields.MPP	= (char)PrivilegeMode::User;
	return { true, true, 0, 0, 0 };
}
ExecReturn exec_SRET(Hart& hart, InstructionData& inst)
{
	if(hart.mode == PrivilegeMode::User || (hart.mode == PrivilegeMode::Supervisor && (hart.status.fields.TSR & 1)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	hart.pc = hart.csr_read(CSR_SEPC);
	switch(hart.status.fields.SPP)
	{
		case 0b0:
			hart.mode = PrivilegeMode::User;
			break;
		case 0b1:
			// If SPP != M-mode, SRET also sets MPRV=0.
			hart.status.fields.MPRV = 0;
			hart.mode				= PrivilegeMode::Supervisor;
			break;
	}
	// Read a previous interrupt-enable bit for supervisor mode (SPIE,5), and set a global interrupt-enable bit for supervisor mode (SIE, 1) to it.
	hart.status.fields.SIE	= hart.status.fields.SPIE;
	hart.status.fields.SPIE = 1;
	hart.status.fields.SPP	= (char)PrivilegeMode::User;
	return { true, true, 0, 0, 0 };
}
ExecReturn exec_SFENCE_VMA(Hart& hart, InstructionData& inst)
{
	// Clear TLB Cache

	bool tvm = hart.status.fields.TVM;

	// Only legal in S or M mode
	if(hart.mode == PrivilegeMode::User)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(hart.mode == PrivilegeMode::Supervisor && tvm)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	// tlb_flush(hart.mmio->mmu->tlb);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_WFI(Hart& hart, InstructionData& inst)
{
	bool tw = hart.status.fields.TW;

	if(tw)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if((hart.ip.raw & hart.ie.raw) == 0)
	{
		clock_t now = clock();

		uint64_t offset;

		/*if (hart.ie & (1U << IRQ_MTIMER)) {
			offset = timecmp_delay_ns(&hart.aclint->mtimecmp[hart.id]);
		}
		if (hart.ie & (1U << IRQ_STIMER)) {
			offset = min(UINT64_MAX,timecmp_delay_ns(&hart.stimecmp));
		}

		clock_t duration = (offset * CLOCKS_PER_SEC) / 1000000000LL;

		//cout << timer_get(&hart.aclint->mtime) << "  " << timecmp_get(&hart.stimecmp) << "  " << offset << "   " << duration << endl;

		hart.wfi_timer = now + duration;*/
		hart.WFI = true;
	}
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_priv()
{
	register_instr("00110000001000000000000001110011", exec_MRET);
	register_instr("00010000001000000000000001110011", exec_SRET);
	register_instr("00010000010100000000000001110011", exec_WFI);
	register_instr("0001001**********000000001110011", exec_SFENCE_VMA);
}
