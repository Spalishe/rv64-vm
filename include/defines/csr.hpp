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
#include <cstdint>

// User Floating-Point CSRs
#define CSR_FFLAGS 0x001 // URW Floating-Point Accrued Exceptions.
#define CSR_FRM	   0x002 // URW Floating-Point Dynamic Rounding Mode.
#define CSR_FCSR \
	0x003 // URW Floating-Point Control and Status Register (frm + fflags)

// User Counter/Timers
#define CSR_CYCLE 0xC00 // URO Cycle counter for RDCYCLE instruction.
#define CSR_TIME  0xC01 // URO Timer for RDTIME instruction.
#define CSR_INSTRET \
	0xC02					   // URO Instructions-retired counter for RDINSTRET instruction.
#define CSR_HPMCOUNTER3	 0xC03 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER4	 0xC04 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER5	 0xC05 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER6	 0xC06 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER7	 0xC07 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER8	 0xC08 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER9	 0xC09 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER10 0xC0A // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER11 0xC0B // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER12 0xC0C // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER13 0xC0D // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER14 0xC0E // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER15 0xC0F // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER16 0xC10 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER17 0xC11 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER18 0xC12 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER19 0xC13 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER20 0xC14 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER21 0xC15 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER22 0xC16 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER23 0xC17 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER24 0xC18 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER25 0xC19 // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER26 0xC1A // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER27 0xC1B // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER28 0xC1C // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER29 0xC1D // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER30 0xC1E // URO Performance-monitoring counter.
#define CSR_HPMCOUNTER31 0xC1F // URO Performance-monitoring counter.

// Supervisor Trap Setup
#define CSR_SSTATUS		 0x100 // SRW Supervisor status register.
#define CSR_SEDELEG		 0x102 // SRW Supervisor exception delegation register.
#define CSR_SIDELEG		 0x103 // SRW Supervisor interrupt delegation register.
#define CSR_SIE			 0x104 // SRW Supervisor interrupt-enable register.
#define CSR_STVEC		 0x105 // SRW Supervisor trap handler base address.
#define CSR_SCOUNTEREN	 0x106 // SRW Supervisor counter enable.
#define CSR_SCOUNTOVF	 0xDA0 // SRW Supervisor counter overflow register

// Ssstateen
#define CSR_SSTATEEN0	 0x10C // SRW Supervisor state enable register
#define CSR_SSTATEEN1	 0x10D // SRW Supervisor state enable register
#define CSR_SSTATEEN2	 0x10E // SRW Supervisor state enable register
#define CSR_SSTATEEN3	 0x10F // SRW Supervisor state enable register

// Supervisor Trap Handling
#define CSR_SSCRATCH	 0x140 // SRW Scratch register for supervisor trap handlers.
#define CSR_SEPC		 0x141 // SRW Supervisor exception program counter.
#define CSR_SCAUSE		 0x142 // SRW Supervisor trap cause.
#define CSR_STVAL		 0x143 // SRW Supervisor bad address or instruction.
#define CSR_SIP			 0x144 // SRW Supervisor interrupt pending.

// https://lists.riscv.org/g/tech-privileged/topic/fast_track_stimecmp/78649672
#define CSR_STIMECMP	 0x14D // SRW Supervisor timer compare
#define CSR_STIMECMPH	 0x15D // SRW Supervisor timer compare high

// Supervisor Protection and Translation
#define CSR_SATP		 0x180 // SRW Supervisor address translation and protection.

// Machine Information Registers
#define CSR_MVENDORID	 0xF11 // MRO Vendor ID.
#define CSR_MARCHID		 0xF12 // MRO Architecture ID.
#define CSR_MIMPID		 0xF13 // MRO Implementation ID.
#define CSR_MHARTID		 0xF14 // MRO Hardware thread ID.

// Machine Trap Setup
#define CSR_MSTATUS		 0x300 // MRW Machine status register.
#define CSR_MISA		 0x301 // MRW ISA and extensions
#define CSR_MEDELEG		 0x302 // MRW Machine exception delegation register.
#define CSR_MIDELEG		 0x303 // MRW Machine interrupt delegation register.
#define CSR_MIE			 0x304 // MRW Machine interrupt-enable register.
#define CSR_MTVEC		 0x305 // MRW Machine trap-handler base address.
#define CSR_MCOUNTEREN	 0x306 // MRW Machine counter enable.

// Smstateen
#define CSR_MSTATEEN0	 0x30C // MRW Machine state enable register
#define CSR_MSTATEEN1	 0x30D // MRW Machine state enable register
#define CSR_MSTATEEN2	 0x30E // MRW Machine state enable register
#define CSR_MSTATEEN3	 0x30F // MRW Machine state enable register

// Machine cycle and instret privilege mode filtering (Smcntrpmf)
#define CSR_MCYCLECFG	 0x321 // MRW Machine privilege mode filtering for the cycle counter.
#define CSR_MINSTRETCFG	 0x322 // MRW Machine privilege mode filtering for the instret counter.

// Machine Trap Handling
#define CSR_MSCRATCH	 0x340 // MRW Scratch register for machine trap handlers.
#define CSR_MEPC		 0x341 // MRW Machine exception program counter.
#define CSR_MCAUSE		 0x342 // MRW Machine trap cause.
#define CSR_MTVAL		 0x343 // MRW Machine bad address or instruction.
#define CSR_MIP			 0x344 // MRW Machine interrupt pending.

// Machine Memory Protection
#define CSR_PMPCFG0		 0x3A0 // MRW Physical memory protection configuration.
#define CSR_PMPCFG1 \
	0x3A1				  // MRW Physical memory protection configuration, RV32 only.
#define CSR_PMPCFG2 0x3A2 // MRW Physical memory protection configuration.
#define CSR_PMPCFG3 \
	0x3A3				  // MRW Physical memory protection configuration, RV32 only.
#define CSR_PMPCFG4 0x3A4 // MRW Physical memory protection configuration.
#define CSR_PMPCFG5 \
	0x3A5				  // MRW Physical memory protection configuration, RV32 only.
#define CSR_PMPCFG6 0x3A6 // MRW Physical memory protection configuration.
#define CSR_PMPCFG7 \
	0x3A7				  // MRW Physical memory protection configuration, RV32 only.
#define CSR_PMPCFG8 0x3A8 // MRW Physical memory protection configuration.
#define CSR_PMPCFG9 \
	0x3A9				   // MRW Physical memory protection configuration, RV32 only.
#define CSR_PMPCFG10 0x3AA // MRW Physical memory protection configuration.
#define CSR_PMPCFG11 \
	0x3AB				   // MRW Physical memory protection configuration, RV32 only.
#define CSR_PMPCFG12 0x3AC // MRW Physical memory protection configuration.
#define CSR_PMPCFG13 \
	0x3AD				   // MRW Physical memory protection configuration, RV32 only.
#define CSR_PMPCFG14 0x3AE // MRW Physical memory protection configuration.
#define CSR_PMPCFG15 \
	0x3AF // MRW Physical memory protection configuration, RV32 only.

#define CSR_PMPADDR0	  0x3B0 // MRW Physical memory protection address register.
#define CSR_PMPADDR1	  0x3B1 // MRW Physical memory protection address register.
#define CSR_PMPADDR2	  0x3B2 // MRW Physical memory protection address register.
#define CSR_PMPADDR3	  0x3B3 // MRW Physical memory protection address register.
#define CSR_PMPADDR4	  0x3B4 // MRW Physical memory protection address register.
#define CSR_PMPADDR5	  0x3B5 // MRW Physical memory protection address register.
#define CSR_PMPADDR6	  0x3B6 // MRW Physical memory protection address register.
#define CSR_PMPADDR7	  0x3B7 // MRW Physical memory protection address register.
#define CSR_PMPADDR8	  0x3B8 // MRW Physical memory protection address register.
#define CSR_PMPADDR9	  0x3B9 // MRW Physical memory protection address register.
#define CSR_PMPADDR10	  0x3BA // MRW Physical memory protection address register.
#define CSR_PMPADDR11	  0x3BB // MRW Physical memory protection address register.
#define CSR_PMPADDR12	  0x3BC // MRW Physical memory protection address register.
#define CSR_PMPADDR13	  0x3BD // MRW Physical memory protection address register.
#define CSR_PMPADDR14	  0x3BE // MRW Physical memory protection address register.
#define CSR_PMPADDR15	  0x3BF // MRW Physical memory protection address register.
#define CSR_PMPADDR16	  0x3C0 // MRW Physical memory protection address register.
#define CSR_PMPADDR17	  0x3C1 // MRW Physical memory protection address register.
#define CSR_PMPADDR18	  0x3C2 // MRW Physical memory protection address register.
#define CSR_PMPADDR19	  0x3C3 // MRW Physical memory protection address register.
#define CSR_PMPADDR20	  0x3C4 // MRW Physical memory protection address register.
#define CSR_PMPADDR21	  0x3C5 // MRW Physical memory protection address register.
#define CSR_PMPADDR22	  0x3C6 // MRW Physical memory protection address register.
#define CSR_PMPADDR23	  0x3C7 // MRW Physical memory protection address register.
#define CSR_PMPADDR24	  0x3C8 // MRW Physical memory protection address register.
#define CSR_PMPADDR25	  0x3C9 // MRW Physical memory protection address register.
#define CSR_PMPADDR26	  0x3CA // MRW Physical memory protection address register.
#define CSR_PMPADDR27	  0x3CB // MRW Physical memory protection address register.
#define CSR_PMPADDR28	  0x3CC // MRW Physical memory protection address register.
#define CSR_PMPADDR29	  0x3CD // MRW Physical memory protection address register.
#define CSR_PMPADDR30	  0x3CE // MRW Physical memory protection address register.
#define CSR_PMPADDR31	  0x3CF // MRW Physical memory protection address register.
#define CSR_PMPADDR32	  0x3D0 // MRW Physical memory protection address register.
#define CSR_PMPADDR33	  0x3D1 // MRW Physical memory protection address register.
#define CSR_PMPADDR34	  0x3D2 // MRW Physical memory protection address register.
#define CSR_PMPADDR35	  0x3D3 // MRW Physical memory protection address register.
#define CSR_PMPADDR36	  0x3D4 // MRW Physical memory protection address register.
#define CSR_PMPADDR37	  0x3D5 // MRW Physical memory protection address register.
#define CSR_PMPADDR38	  0x3D6 // MRW Physical memory protection address register.
#define CSR_PMPADDR39	  0x3D7 // MRW Physical memory protection address register.
#define CSR_PMPADDR40	  0x3D8 // MRW Physical memory protection address register.
#define CSR_PMPADDR41	  0x3D9 // MRW Physical memory protection address register.
#define CSR_PMPADDR42	  0x3DA // MRW Physical memory protection address register.
#define CSR_PMPADDR43	  0x3DB // MRW Physical memory protection address register.
#define CSR_PMPADDR44	  0x3DC // MRW Physical memory protection address register.
#define CSR_PMPADDR45	  0x3DD // MRW Physical memory protection address register.
#define CSR_PMPADDR46	  0x3DE // MRW Physical memory protection address register.
#define CSR_PMPADDR47	  0x3DF // MRW Physical memory protection address register.
#define CSR_PMPADDR48	  0x3E0 // MRW Physical memory protection address register.
#define CSR_PMPADDR49	  0x3E1 // MRW Physical memory protection address register.
#define CSR_PMPADDR50	  0x3E2 // MRW Physical memory protection address register.
#define CSR_PMPADDR51	  0x3E3 // MRW Physical memory protection address register.
#define CSR_PMPADDR52	  0x3E4 // MRW Physical memory protection address register.
#define CSR_PMPADDR53	  0x3E5 // MRW Physical memory protection address register.
#define CSR_PMPADDR54	  0x3E6 // MRW Physical memory protection address register.
#define CSR_PMPADDR55	  0x3E7 // MRW Physical memory protection address register.
#define CSR_PMPADDR56	  0x3E8 // MRW Physical memory protection address register.
#define CSR_PMPADDR57	  0x3E9 // MRW Physical memory protection address register.
#define CSR_PMPADDR58	  0x3EA // MRW Physical memory protection address register.
#define CSR_PMPADDR59	  0x3EB // MRW Physical memory protection address register.
#define CSR_PMPADDR60	  0x3EC // MRW Physical memory protection address register.
#define CSR_PMPADDR61	  0x3ED // MRW Physical memory protection address register.
#define CSR_PMPADDR62	  0x3EE // MRW Physical memory protection address register.
#define CSR_PMPADDR63	  0x3EF // MRW Physical memory protection address register.

// Machine Counter/Timers
#define CSR_MCYCLE		  0xB00 // MRW Machine cycle counter.
#define CSR_MINSTRET	  0xB02 // MRW Machine instructions-retired counter.
#define CSR_MHPMCOUNTER3  0xB03 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER4  0xB04 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER5  0xB05 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER6  0xB06 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER7  0xB07 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER8  0xB08 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER9  0xB09 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER10 0xB0A // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER11 0xB0B // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER12 0xB0C // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER13 0xB0D // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER14 0xB0E // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER15 0xB0F // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER16 0xB10 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER17 0xB11 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER18 0xB12 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER19 0xB13 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER20 0xB14 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER21 0xB15 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER22 0xB16 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER23 0xB17 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER24 0xB18 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER25 0xB19 // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER26 0xB1A // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER27 0xB1B // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER28 0xB1C // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER29 0xB1D // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER30 0xB1E // MRW Machine performance-monitoring counter.
#define CSR_MHPMCOUNTER31 0xB1F // MRW Machine performance-monitoring counter.

// Machine Counter Setup
#define CSR_MCOUNTINHIBIT 0x320 // MRW Machine counter-inhibit register.
#define CSR_MHPMEVENT3 \
	0x323 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT4 \
	0x324 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT5 \
	0x325 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT6 \
	0x326 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT7 \
	0x327 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT8 \
	0x328 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT9 \
	0x329 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT10 \
	0x32A // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT11 \
	0x32B // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT12 \
	0x32C // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT13 \
	0x32D // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT14 \
	0x32E // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT15 \
	0x32F // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT16 \
	0x330 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT17 \
	0x331 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT18 \
	0x332 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT19 \
	0x333 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT20 \
	0x334 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT21 \
	0x335 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT22 \
	0x336 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT23 \
	0x337 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT24 \
	0x338 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT25 \
	0x339 // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT26 \
	0x33A // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT27 \
	0x33B // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT28 \
	0x33C // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT29 \
	0x33D // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT30 \
	0x33E // MRW Machine performance-monitoring event selector.
#define CSR_MHPMEVENT31 \
	0x33F // MRW Machine performance-monitoring event selector.

// Debug/Trace Registers (shared with Debug Mode)
#define CSR_TSELECT		 0x7A0 // MRW Debug/Trace trigger register select.
#define CSR_TDATA1		 0x7A1 // MRW First Debug/Trace trigger data register.
#define CSR_TDATA2		 0x7A2 // MRW Second Debug/Trace trigger data register.
#define CSR_TDATA3		 0x7A3 // MRW Third Debug/Trace trigger data register.

// Debug Mode Registers
#define CSR_DCSR		 0x7B0 // DRW Debug control and status register.
#define CSR_DPC			 0x7B1 // DRW Debug PC.
#define CSR_DSCRATCH0	 0x7B2 // DRW Debug scratch register 0.
#define CSR_DSCRATCH1	 0x7B3 // DRW Debug scratch register 1.

// MMAIA
#define CSR_MTOPI		 0xFB0 // MRW Top allowed and pending interrupt register

// SSTATUS bits (these are aliases typically)
#define SSTATUS_SIE_BIT	 1
#define SSTATUS_SPIE_BIT 5
#define SSTATUS_SPP_BIT	 8

// mtvec/stvec mode masks
#define TVEC_MODE_MASK	 0x3ULL
#define TVEC_BASE_MASK	 (~0x3ULL)

// MIP/MIE bit positions (spec Figure 3.10/3.11)
#define MIP_MEIP		 11
#define MIP_SEIP		 9
#define MIP_UEIP		 8
#define MIP_MTIP		 7
#define MIP_HTIP		 6
#define MIP_STIP		 5
#define MIP_UTIP		 4
#define MIP_MSIP		 3
#define MIP_HSIP		 2
#define MIP_SSIP		 1
#define MIP_USIP		 0

#define MIE_MEIE_BIT	 11
#define MIE_MSIE_BIT	 3
#define MIE_MTIP_BIT	 7
#define SIE_SEIE_BIT	 9
#define MIE_SEIE_BIT	 9
#define SIE_SSIE_BIT	 1
#define MIE_SSIE_BIT	 1
#define SIE_STIE_BIT	 5
#define MIE_STIE_BIT	 5
#define SIP_STIP_BIT	 5

#define MSTATUS_MPRV	 17
#define MSTATUS_MPP_LOW	 11
#define MSTATUS_MPP_HIGH 12
#define MSTATUS_MIE		 3
#define MSTATUS_MPIE	 7
#define MSTATUS_TSR		 22
#define MSTATUS_SPP		 8
#define MSTATUS_SIE		 1
#define MSTATUS_SPIE	 5
#define MSTATUS_MXR		 19
#define MSTATUS_SUM		 18

#define SSTATUS_MASK	 0x80000003000DE762ULL
#define SE_MASK			 0x222

union ip_t
{
	struct
	{
		uint16_t : 1;
		uint16_t SSIP : 1;
		uint16_t : 1;
		uint16_t MSIP : 1;
		uint16_t : 1;
		uint16_t STIP : 1;
		uint16_t : 1;
		uint16_t MTIP : 1;
		uint16_t : 1;
		uint16_t SEIP : 1;
		uint16_t : 1;
		uint16_t MEIP : 1;
		uint16_t : 4;
	} fields;
	uint16_t raw;
};
union ie_t
{
	struct
	{
		uint16_t : 1;
		uint16_t SSIE : 1;
		uint16_t : 1;
		uint16_t MSIE : 1;
		uint16_t : 1;
		uint16_t STIE : 1;
		uint16_t : 1;
		uint16_t MTIE : 1;
		uint16_t : 1;
		uint16_t SEIE : 1;
		uint16_t : 1;
		uint16_t MEIE : 1;
		uint16_t : 4;
	} fields;
	uint16_t raw;
};

#define MSTATUS_RO_MASK 0xF00000000ULL

union status_t
{
	struct
	{
		uint64_t : 1;
		uint64_t SIE : 1;
		uint64_t : 1;
		uint64_t MIE : 1;
		uint64_t : 1;
		uint64_t SPIE : 1;
		uint64_t UBE : 1;
		uint64_t MPIE : 1;
		uint64_t SPP : 1;
		uint64_t VS : 2;
		uint64_t MPP : 2;
		uint64_t FS : 2;
		uint64_t XS : 2;
		uint64_t MPRV : 1;
		uint64_t SUM : 1;
		uint64_t MXR : 1;
		uint64_t TVM : 1;
		uint64_t TW : 1;
		uint64_t TSR : 1;
		uint64_t : 9;
		uint64_t UXL : 2;
		uint64_t SXL : 2;
		uint64_t SBE : 1;
		uint64_t MBE : 1;
		uint64_t : 25;
		uint64_t SD : 1;
	} fields;

	uint64_t raw;
};
union fcsr_t
{
	struct
	{
		uint64_t fflags : 5;
		uint64_t frm : 3;
		uint64_t : 56;
	} fields;

	uint64_t raw;
};

union satp_t
{
	struct
	{
		uint64_t ppn : 44;	// Root page table physical number aligned by 4096
		uint64_t asid : 16; // Address space identifier
		uint64_t mode : 4;
	} fields;
	uint64_t raw;
};
