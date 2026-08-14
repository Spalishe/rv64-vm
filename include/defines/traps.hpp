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

// mcause / scause exception codes (Interrupt bit is set separately)
#define EXC_INST_ADDR_MISALIGNED  0
#define EXC_INST_ACCESS_FAULT	  1
#define EXC_ILLEGAL_INSTRUCTION	  2
#define EXC_BREAKPOINT			  3
#define EXC_LOAD_ADDR_MISALIGNED  4
#define EXC_LOAD_ACCESS_FAULT	  5
#define EXC_STORE_ADDR_MISALIGNED 6
#define EXC_STORE_ACCESS_FAULT	  7
#define EXC_ENV_CALL_FROM_U		  8
#define EXC_ENV_CALL_FROM_S		  9
#define EXC_ENV_CALL_FROM_M		  11
#define EXC_INST_PAGE_FAULT		  12
#define EXC_LOAD_PAGE_FAULT		  13
#define EXC_STORE_PAGE_FAULT	  15

// Interrupt cause codes (these are the Exception_Code field when Interrupt bit
// = 1)
#define IRQ_USW					  0 // user software
#define IRQ_SSW					  1 // supervisor software
#define IRQ_MSW					  3 // machine software
#define IRQ_UTIMER				  4
#define IRQ_STIMER				  5
#define IRQ_MTIMER				  7
#define IRQ_UEXT				  8
#define IRQ_SEXT				  9
#define IRQ_MEXT				  11

static const int irq_priority[] = {
	11, // MEI
	9,	// SEI
	3,	// MSI
	1,	// SSI
	7,	// MTI
	5	// STI
};

struct ExecReturn
{
	bool is_success	   = true;
	bool can_change_pc = false;
	char increase_pc   = 4;
	char cause		   = 0;
	uint64_t tval	   = 0;
};
enum class MemorySize : int
{
	Byte  = 1,
	Short = 2,
	Int	  = 4,
	Long  = 8
};
struct MemoryReturn
{
	bool is_success = true;
	char exc_code	= 0;
	uint64_t tval	= 0;
};
enum class AccessType : int
{
	LOAD  = 0,
	STORE = 1,
	EXEC  = 2
};
