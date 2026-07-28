/*
* Copyright 2026 Spalishe

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
#include "rvjit.hpp"
#ifdef USE_JIT
#include "rvjit_cfg.hpp"
#include "rvjit_emit.hpp"
#include <cstdint>
#ifdef HOST_TARGET_X86_64

struct JIT_Emitter;

// ABI contract:
// rdi = JIT_HartContext*
// r12 = ctx
// r13 = regs*
constexpr uint8_t REG_RAX = 0b0000;
constexpr uint8_t REG_RCX = 0b0001;
constexpr uint8_t REG_RDX = 0b0010;
constexpr uint8_t REG_RBX = 0b0011;
constexpr uint8_t REG_RSP = 0b0100;
constexpr uint8_t REG_RBP = 0b0101;
constexpr uint8_t REG_RSI = 0b0110;
constexpr uint8_t REG_RDI = 0b0111;
constexpr uint8_t REG_R8  = 0b1000;
constexpr uint8_t REG_R9  = 0b1001;
constexpr uint8_t REG_R10 = 0b1010;
constexpr uint8_t REG_R11 = 0b1011;
constexpr uint8_t REG_R12 = 0b1100;
constexpr uint8_t REG_R13 = 0b1101;
constexpr uint8_t REG_R14 = 0b1110;
constexpr uint8_t REG_R15 = 0b1111;

/*
 *	MOD:
 *		11: both registers
 *		00: memory without offset ([reg] or [base+index*scale])
 *		01: memory + disp8
 *		10: memory + disp32
 *	REG & RM: (REG is like rs1, RM is rd)
 *		000: RAX   R8
 *		001: RCX   R9
 *		010: RDX   R10
 *		011: RBX   R11
 *		100: RSP   R12
 *		101: RBP   R13
 *		110: RSI   R14
 *		111: RDI   R15
 */
inline uint8_t
modrm(uint8_t mod, uint8_t reg, uint8_t rm)
{
	return (mod << 6) | ((reg & 7) << 3) | (rm & 7);
}
/*
 *	Scale is basically multiplier
 *		00: *1
 *		01: *2
 *		10: *4
 *		11: *8
 *	Index and base are same registers
 */
inline uint8_t sib(char scale, char index, char base)
{
	if(index == -1)
		index = 4; // no index
	return (scale << 6) | ((index & 7) << 3) | (base & 7);
}

/*
 *	W: Operand size
 *		0 - default
 *		1 - 64-bit
 *	R: Extends REG field in ModRM
 *		allows to use r8-r15
 *	X: Extends index register in SIB
 *	B: Extends RM field in ModRM, or BASE field in SIB
 */
inline uint8_t rex(bool W, bool R, bool X, bool B)
{
	return (0b0100 << 4) | (W << 3) | (R << 2) | (X << 1) | B;
}

inline bool needs_disp0(uint8_t base)
{
	return base != REG_RBP && base != REG_R13;
}
#define NO_INDEX 0xFF
inline bool needs_sib(uint8_t base, uint8_t index)
{
	return index != NO_INDEX || (base & 7) == 4; // rsp/r12
}

// SIB helper
inline void sib_helper(JIT_Block& blk, uint8_t reg, uint8_t base, uint8_t index, uint8_t scale, int32_t disp)
{
	bool has_index = index != 0xFF;
	bool need_sib  = needs_sib(base, index);
	uint8_t mod;
	if(disp == 0 && needs_disp0(base))
		mod = 0;
	else if(disp >= -128 && disp <= 127)
		mod = 1;
	else
		mod = 2;
	blk.bytes[blk.byte_pos++] = modrm(mod, reg & 7, need_sib ? 4 : (base & 7));

	if(need_sib)
	{
		blk.bytes[blk.byte_pos++] = sib(scale, has_index ? (index & 7) : 4, base & 7);
	}

	if(mod == 1)
	{
		blk.bytes[blk.byte_pos++] = (uint8_t)disp;
	}
	else if(mod == 2)
	{
		blk.bytes[blk.byte_pos++] = disp & 0xFF;
		blk.bytes[blk.byte_pos++] = (disp >> 8) & 0xFF;
		blk.bytes[blk.byte_pos++] = (disp >> 16) & 0xFF;
		blk.bytes[blk.byte_pos++] = (disp >> 24) & 0xFF;
	}
}

// SUB r/m64, r64
inline void sub_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x29;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// SUB r/m32, r32
inline void sub_rr32(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(0, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x29;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// SUB r64, imm32
inline void sub_rimm32(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, imm32 is 5
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b101, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// SUB r/m64, imm32
inline void sub_mimm32(
	JIT_Block& blk,
	uint8_t reg_base,
	uint8_t reg_index,
	uint8_t scale,
	int32_t disp,
	int32_t imm32)
{
	blk.bytes[blk.byte_pos++] = rex(1, 0,
									(reg_index != 0xFF && reg_index > 7),
									(reg_base > 7));

	blk.bytes[blk.byte_pos++] = 0x81;

	// reg field = 5 => SUB
	sib_helper(blk, 5, reg_base, reg_index, scale, disp);

	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// SUB r/m64, imm32
inline void sub_riw(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, REG must be 0b101
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b101, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// ADD r/m64, r64
inline void add_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x01;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// ADD r/m64, imm32
inline void add_rimm32(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, imm32 is REG
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b000, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// ADD r/m32, imm32
inline void add_r32imm32(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, imm32 is REG
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b000, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// ADD r64, imm8
inline void add_r64imm8(JIT_Block& blk, char dest, uint8_t imm)
{
	// dest is RM, source is
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x83;
	blk.bytes[blk.byte_pos++] = modrm(3, 0, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm;
}
// ADD r/m32, r32
inline void add_rr32(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(0, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x01;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// ADD r/m64, imm32
inline void add_riw(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, REG must be 0
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// NEG r64
inline void neg(JIT_Block& blk, char dest)
{
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xF7;
	blk.bytes[blk.byte_pos++] = modrm(3, 3, (dest & 7));
}
// NEG r32
inline void neg32(JIT_Block& blk, char dest)
{
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xF7;
	blk.bytes[blk.byte_pos++] = modrm(3, 3, (dest & 7));
}

// XOR r/m64, r64
inline void xor_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x31;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// XOR r/m32, r32
inline void xor_rr32(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(0, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x31;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// XOR r/m64, imm32
inline void xor_rimm32(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, imm is 6
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b110, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// OR r/m64, r64
inline void or_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x09;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// OR r/m64, imm32
inline void or_rimm32(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, imm is 1
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b001, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// AND r/m64, r64
inline void and_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x21;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// AND r/m64, imm32
inline void and_rimm32(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, imm32 is 4
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b100, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// MOV r/m64, r64
inline void mov(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x89;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// MOV r/m64, imm32
inline void mov_imm32(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM,  REG must be 0
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xC7;
	blk.bytes[blk.byte_pos++] = modrm(3, 0, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// MOV r/m64, imm64
inline void mov_imm64(JIT_Block& blk, char dest, int64_t imm64)
{
	// dest is RM, REG must be 0
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xB8 + (dest & 7);
	for(int i = 0; i < 8; i++)
		blk.bytes[blk.byte_pos++] = (imm64 >> (i * 8)) & 0xFF;
}
// MOV memory8,r8
inline void mov_m8r8(JIT_Block& blk, uint8_t source, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(0, (source > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x88;
	sib_helper(blk, source, reg_base, reg_index, scale, disp);
}
// MOV memory16,r16
inline void mov_m16r16(JIT_Block& blk, uint8_t source, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = 0x66;
	blk.bytes[blk.byte_pos++] = rex(0, (source > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x89;
	sib_helper(blk, source, reg_base, reg_index, scale, disp);
}
// MOV memory32,r32
inline void mov_m32r32(JIT_Block& blk, uint8_t source, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(0, (source > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x89;
	sib_helper(blk, source, reg_base, reg_index, scale, disp);
}
// MOV memory64,r64
inline void mov_mr(JIT_Block& blk, uint8_t source, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x89;
	sib_helper(blk, source, reg_base, reg_index, scale, disp);
}
// MOV r64,memory
inline void mov_rm(JIT_Block& blk, uint8_t dest, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (dest > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x8B;
	sib_helper(blk, dest, reg_base, reg_index, scale, disp);
}
// MOV r32,memory
inline void mov_r32m(JIT_Block& blk, uint8_t dest, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(0, (dest > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x8B;
	sib_helper(blk, dest, reg_base, reg_index, scale, disp);
}
// MOVZX r32, r/m8
inline void movzx_r32m8(JIT_Block& blk,
						uint8_t dest,
						uint8_t base,
						uint8_t index,
						uint8_t scale,
						int32_t disp = 0)
{
	blk.bytes[blk.byte_pos++] = rex(0, dest > 7, index > 7, base > 7);

	blk.bytes[blk.byte_pos++] = 0x0F;
	blk.bytes[blk.byte_pos++] = 0xB6;

	sib_helper(blk, dest, base, index, scale, disp);
}
// MOVZX r32, r/m16
inline void movzx_r32m16(JIT_Block& blk,
						 uint8_t dest,
						 uint8_t base,
						 uint8_t index,
						 uint8_t scale,
						 int32_t disp = 0)
{
	blk.bytes[blk.byte_pos++] = rex(0, dest > 7, index > 7, base > 7);

	blk.bytes[blk.byte_pos++] = 0x0F;
	blk.bytes[blk.byte_pos++] = 0xB7;

	sib_helper(blk, dest, base, index, scale, disp);
}
// MOVSX r64, r/m8
inline void movsx_r64m8(JIT_Block& blk,
						uint8_t dest,
						uint8_t base,
						uint8_t index,
						uint8_t scale,
						int32_t disp = 0)
{
	blk.bytes[blk.byte_pos++] = rex(1, dest > 7, index > 7, base > 7);

	blk.bytes[blk.byte_pos++] = 0x0F;
	blk.bytes[blk.byte_pos++] = 0xBE;

	sib_helper(blk, dest, base, index, scale, disp);
}
// MOVSX r64, r/m16
inline void movsx_r64m16(JIT_Block& blk,
						 uint8_t dest,
						 uint8_t base,
						 uint8_t index,
						 uint8_t scale,
						 int32_t disp = 0)
{
	blk.bytes[blk.byte_pos++] = rex(1, dest > 7, index > 7, base > 7);

	blk.bytes[blk.byte_pos++] = 0x0F;
	blk.bytes[blk.byte_pos++] = 0xBF;

	sib_helper(blk, dest, base, index, scale, disp);
}
// MOVSXD r64, r/m32
inline void movsxd_r64m32(JIT_Block& blk,
						  uint8_t dest,
						  uint8_t base,
						  uint8_t index,
						  uint8_t scale,
						  int32_t disp = 0)
{
	blk.bytes[blk.byte_pos++] = rex(1, dest > 7, index > 7, base > 7);

	blk.bytes[blk.byte_pos++] = 0x63;

	sib_helper(blk, dest, base, index, scale, disp);
}
// MOVSXD r64, r/m32
inline void movsxd(JIT_Block& blk, char dest, char source)
{
	// dest is REG, source is RM
	blk.bytes[blk.byte_pos++] = rex(1, (dest > 7), 0, (source > 7));
	blk.bytes[blk.byte_pos++] = 0x63;
	blk.bytes[blk.byte_pos++] = modrm(3, (dest & 7), (source & 7));
}
using MovSignature = void (*)(JIT_Block&, uint8_t, uint8_t, uint8_t, uint8_t, int32_t);

// SHL r/m64, CL
// CL is RCX(first 6 bits)
inline void shl_rc(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	mov(blk, REG_RCX, source);
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xD3;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b100, (dest & 7));
}
// SHL r/m64, imm8
inline void shl_rimm8(JIT_Block& blk, char dest, uint8_t imm8)
{
	// dest is RM, imm8 is 4
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xC1;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b100, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm8 & 0xFF;
}
// SHR r/m64, CL
// CL is RCX(first 6 bits)
inline void shr_rc(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	mov(blk, REG_RCX, source);
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xD3;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b101, (dest & 7));
}
// SHR r/m64, imm8
inline void shr_rimm8(JIT_Block& blk, char dest, uint8_t imm8)
{
	// dest is RM, imm8 is 5
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xC1;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b101, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm8 & 0xFF;
}
// SAR r/m64, CL
// CL is RCX(first 6 bits)
inline void sar_rc(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	mov(blk, REG_RCX, source);
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xD3;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b111, (dest & 7));
}
// SAR r/m64, imm8
inline void sar_rimm8(JIT_Block& blk, char dest, uint8_t imm8)
{
	// dest is RM, imm8 is 7
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xC1;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b111, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm8 & 0xFF;
}

// SHL r/m32, CL
// CL is RCX(first 5 bits)
inline void shl_rc32(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	mov(blk, REG_RCX, source);
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xD3;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b100, (dest & 7));
}
// SHL r/m32, imm8
inline void shl_r32imm8(JIT_Block& blk, char dest, uint8_t imm8)
{
	// dest is RM, imm8 is 4
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xC1;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b100, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm8 & 0xFF;
}

// SHR r/m32, CL
// CL is RCX(first 5 bits)
inline void shr_rc32(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	mov(blk, REG_RCX, source);
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xD3;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b101, (dest & 7));
}
// SHR r/m32, imm8
inline void shr_r32imm8(JIT_Block& blk, char dest, uint8_t imm8)
{
	// dest is RM, imm8 is 5
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xC1;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b101, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm8 & 0xFF;
}
// SAR r/m32, CL
// CL is RCX(first 5 bits)
inline void sar_rc32(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	mov(blk, REG_RCX, source);
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xD3;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b111, (dest & 7));
}
// SAR r/m32, imm8
inline void sar_r32imm8(JIT_Block& blk, char dest, uint8_t imm8)
{
	// dest is RM, imm8 is 7
	blk.bytes[blk.byte_pos++] = rex(0, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0xC1;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b111, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm8 & 0xFF;
}
// TEST r/m64,r64
inline void test(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, dest > 7);
	blk.bytes[blk.byte_pos++] = 0x85;
	blk.bytes[blk.byte_pos++] = modrm(3, source & 7, dest & 7);
}
// CMP r/m64,r64
inline void cmp(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, dest > 7);
	blk.bytes[blk.byte_pos++] = 0x39;
	blk.bytes[blk.byte_pos++] = modrm(3, source & 7, dest & 7);
}
// CMP r64,r/m64
inline void cmp_rm(JIT_Block& blk, char dest, char reg_base, char reg_index, char scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, dest > 7, reg_index > 7, reg_base > 7);

	blk.bytes[blk.byte_pos++] = 0x3B;

	sib_helper(
		blk,
		dest,
		reg_base,
		reg_index,
		scale,
		disp);
}
// CMP r/m64,r64
inline void cmp_mr(JIT_Block& blk, char dest, char reg_base, char reg_index, char scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, dest > 7, reg_index > 7, reg_base > 7);

	blk.bytes[blk.byte_pos++] = 0x3B;

	sib_helper(
		blk,
		dest,
		reg_base,
		reg_index,
		scale,
		disp);
}
// SETL r/m8
inline void setl(JIT_Block& blk, char dest)
{
	// dest is RM
	if(dest > 3)
		blk.bytes[blk.byte_pos++] = rex(0, 0, 0, dest > 7);

	blk.bytes[blk.byte_pos++] = 0x0F;
	blk.bytes[blk.byte_pos++] = 0x9C;
	blk.bytes[blk.byte_pos++] = modrm(3, 0, dest & 7);
}
// SETB r/m8
inline void setb(JIT_Block& blk, char dest)
{
	// dest is RM
	if(dest > 3)
		blk.bytes[blk.byte_pos++] = rex(0, 0, 0, dest > 7);

	blk.bytes[blk.byte_pos++] = 0x0F;
	blk.bytes[blk.byte_pos++] = 0x92;
	blk.bytes[blk.byte_pos++] = modrm(3, 0, dest & 7);
}
using Jmp8Signature = void (*)(JIT_Block&, int8_t);
// JE rel8
inline void je8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x74;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JNE rel8
inline void jne8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x75;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JL rel8
inline void jl8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x7C;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JB rel8
inline void jb8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x72;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JGE rel8
inline void jge8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x7D;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JAE rel8
inline void jae8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x73;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JBE rel8
inline void jbe8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x76;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JMP rel8
inline void jmp8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0xEB;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JMP rel32
inline void jmp32(JIT_Block& blk, int32_t rel32)
{
	blk.bytes[blk.byte_pos++] = 0xE9;
	blk.bytes[blk.byte_pos++] = rel32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (rel32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (rel32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (rel32 >> 24) & 0xFF;
}
// JS rel8
inline void js8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x78;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JNS rel8
inline void jns8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x79;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JG rel8
inline void jg8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x7F;
	blk.bytes[blk.byte_pos++] = rel8;
}
// JLE rel8
inline void jle8(JIT_Block& blk, int8_t rel8)
{
	blk.bytes[blk.byte_pos++] = 0x7E;
	blk.bytes[blk.byte_pos++] = rel8;
}
// MOVZX r64, r/m8
inline void movzx(JIT_Block& blk, char dest, char source)
{
	// dest is RM
	blk.bytes[blk.byte_pos++] = rex(1, (dest > 7), 0, source > 7);
	blk.bytes[blk.byte_pos++] = 0x0F;
	blk.bytes[blk.byte_pos++] = 0xB6;
	blk.bytes[blk.byte_pos++] = modrm(3, dest & 7, source & 7);
}
// PUSH r64
inline void push(JIT_Block& blk, char reg)
{
	if(reg > 7)
		blk.bytes[blk.byte_pos++] = rex(0, 0, 0, 1);
	blk.bytes[blk.byte_pos++] = 0x50 + (reg & 7);
	;
}
// POP r64
inline void pop(JIT_Block& blk, char reg)
{
	if(reg > 7)
		blk.bytes[blk.byte_pos++] = rex(0, 0, 0, 1);
	blk.bytes[blk.byte_pos++] = 0x58 + (reg & 7);
}
// RET
inline void ret(JIT_Block& blk)
{
	blk.bytes[blk.byte_pos++] = 0xC3;
}
// CALL
inline void call(JIT_Block& blk, char reg)
{
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (reg >> 3) & 1);
	blk.bytes[blk.byte_pos++] = 0xFF;
	blk.bytes[blk.byte_pos++] = modrm(0b11, 2, reg & 7);
}

inline void JIT_Emitter::rvjit_emit_prologue(JIT_Block& blk)
{
	push(blk, REG_R12);
	push(blk, REG_R13);
	push(blk, REG_R14);
	mov(blk, REG_R12, REG_RDI);													// Mov hart context to R12
	mov_rm(blk, REG_R13, REG_R12, NO_INDEX, 0, 0);								// Mov regs from hart context to R13
	mov_rm(blk, REG_R14, REG_R12, NO_INDEX, 0, offsetof(JIT_HartContext, ram)); // Mov ram* from hart context to R1$
}
inline void JIT_Emitter::rvjit_emit_epilogue(JIT_Block& blk)
{
	realize_label(blk, "epilogue");
	realize_label(blk, "branch");
	for(auto& vreg : vregs)
	{
		if(!vreg.allocated)
			continue;

		if(!vreg.dirty)
			continue;

		if(vreg.is_zero)
			continue;

		// store guest reg back
		mov_mr(blk, vreg.host_reg, REG_R13, NO_INDEX, 0, vreg.vreg * 8);
	}

	pop(blk, REG_R14);
	pop(blk, REG_R13); // pop hart regs
	pop(blk, REG_R12); // pop hart context from r12
	ret(blk);
}
inline void JIT_Emitter::reset()
{
	constexpr uint8_t array[] = { REG_RAX, REG_RDX, REG_RSI, REG_RDI, REG_R8, REG_R9, REG_R10, REG_R11 };
	global_use_counter		  = 0;
	for(int i = 0; i < HOST_REGS_COUNT; i++)
	{
		host_regs[i].host_reg = array[i];
		host_regs[i].last_use = 0;
		host_regs[i].vreg	  = 0xFF;
		host_regs[i].used	  = false;
		host_regs[i].idx	  = i;
	}
	for(int i = 0; i < 32; i++)
	{
		vregs[i].allocated = false;
		vregs[i].host_reg  = 0xFF;
		vregs[i].host_idx  = 0;
		vregs[i].vreg	   = i;
		vregs[i].dirty	   = false;
		vregs[i].valid	   = false;
		vregs[i].is_zero   = (i == 0);
	}
}
inline void JIT_Emitter::realize_label(JIT_Block& blk, const std::string& label)
{
	uint32_t cur_pos = blk.byte_pos;

	for(size_t i = 0; i < blk.jmp_labels.size();)
	{
		auto& lbl = blk.jmp_labels[i];

		if(lbl.label != label)
		{
			i++;
			continue;
		}

		uint32_t patch_pos = lbl.offs + (lbl.is_opcode_2 ? 2 : 1);
		uint32_t insn_size = lbl.size + (lbl.is_opcode_2 ? 2 : 1);

		if(lbl.determined_pos != INT64_MIN)
		{
			int64_t target = lbl.determined_pos;

			if(target >= 0 && target < RVJIT_FUNC_SIZE)
			{
				uint64_t host = blk.inst_addr_jmp[target];

				if(host != UINT64_MAX)
				{
					// fast path
					cur_pos = host;
				}
				else
				{
					// slow path
					cur_pos = lbl.offs + insn_size;
				}
			}
			else
			{
				cur_pos = lbl.offs + insn_size;
			}
		}

		if(lbl.size == 1)
		{
			// rel8
			int8_t rel = (int8_t)(cur_pos - (lbl.offs + insn_size));
			std::memcpy(&blk.bytes[patch_pos], &rel, sizeof(int8_t));
		}
		else if(lbl.size == 4)
		{
			// rel32
			int32_t rel = (int32_t)(cur_pos - (lbl.offs + insn_size));
			std::memcpy(&blk.bytes[patch_pos], &rel, sizeof(int32_t));
		}

		blk.jmp_labels.erase(blk.jmp_labels.begin() + i);

		continue;
	}
}
inline void JIT_Emitter::ensure_loaded(JIT_Block& blk, VReg& vreg)
{
	if(vreg.allocated && !vreg.valid)
	{
		mov_rm(blk, vreg.host_reg, REG_R13, NO_INDEX, 0, vreg.vreg * 8);
		vreg.valid = true;
	}
}
inline HReg* JIT_Emitter::spill(JIT_Block& blk, uint64_t locked)
{
	// Gonna spill random one
	HReg* victim = nullptr;

	for(auto& reg : host_regs)
	{
		if(locked & (1ULL << reg.host_reg))
			continue;
		if(victim == nullptr)
			victim = &reg;

		if(reg.last_use < victim->last_use)
			victim = &reg;
	}
	if(victim == nullptr) return victim;
	victim->used = false;
	VReg& vreg	 = vregs[victim->vreg];
	if(vreg.dirty)
		mov_mr(blk, victim->host_reg, REG_R13, NO_INDEX, 0, victim->vreg * 8);
	vreg.allocated = false;
	vreg.valid	   = false;
	vreg.dirty	   = false;
	return victim;
}
inline void JIT_Emitter::flush_all(JIT_Block& blk)
{
	for(auto& reg : host_regs)
	{
		if(!reg.used)
			continue;
		VReg& vreg = vregs[reg.vreg];
		if(vreg.dirty)
			mov_mr(blk, reg.host_reg, REG_R13, NO_INDEX, 0, reg.vreg * 8);
	}
}
inline void JIT_Emitter::invalidate_all()
{
	for(auto& reg : host_regs)
	{
		if(reg.used)
		{
			if(!reg.used)
				continue;
			reg.used	   = false;
			VReg& vreg	   = vregs[reg.vreg];
			vreg.allocated = false;
			vreg.valid	   = false;
			vreg.dirty	   = false;
		}
	}
}

inline VReg& JIT_Emitter::rvjit_alloc_reg(JIT_Block& blk, uint8_t user_reg, uint64_t locked)
{
	// Check if requested user_reg == x0
	if(user_reg == 0)
	{
		return vregs[0];
	}
	// Check if there's already allocated
	if(vregs[user_reg].allocated)
	{
		global_use_counter++;
		host_regs[vregs[user_reg].host_idx].last_use = global_use_counter;
		ensure_loaded(blk, vregs[user_reg]);
		return vregs[user_reg];
	}

	// Find some free host register
	for(auto& hreg : host_regs)
	{
		if(locked & (1u << hreg.host_reg))
			continue;
		if(!hreg.used)
		{
			hreg.used	   = true;
			hreg.vreg	   = user_reg;
			auto& vreg	   = vregs[user_reg];
			vreg.host_reg  = hreg.host_reg;
			vreg.allocated = true;
			ensure_loaded(blk, vreg);
			return vreg;
		}
	}

	// Seems we dont have any free host reg
	HReg* victim	   = spill(blk, locked);
	victim->vreg	   = user_reg;
	victim->used	   = true;
	VReg& new_vreg	   = vregs[user_reg];
	new_vreg.valid	   = false;
	new_vreg.allocated = true;
	new_vreg.host_reg  = victim->host_reg;
	new_vreg.host_idx  = victim->idx;
	ensure_loaded(blk, new_vreg);
	return new_vreg;
}

// Instruction emit part
inline void JIT_Emitter::inst_emit_r_type(Hart& h, InstructionData& inst, JIT_Block& blk, bool optimize_if_rsz, ROpFunction emit_op, uint64_t pc, void* tmp)
{
	blk.inst_addr_jmp[blk.size] = blk.byte_pos;
	if(inst.rd == 0)
	{
		// x0 write ignored
		return;
	}

	bool rs1_zero = optimize_if_rsz ? (inst.rs1 == 0) : false;
	bool rs2_zero = optimize_if_rsz ? (inst.rs2 == 0) : false;

	VReg& rd = rvjit_alloc_reg(blk, inst.rd, 0);
	if(rs1_zero && rs2_zero)
	{
		xor_rr(blk, rd.host_reg, rd.host_reg);
		rd.dirty = true;
		return;
	}

	if(rs1_zero)
	{
		VReg& rs2 = rvjit_alloc_reg(blk, inst.rs2, (1ULL << rd.host_reg));
		mov(blk, rd.host_reg, rs2.host_reg);
		rd.dirty = true;
		return;
	}

	if(rs2_zero)
	{
		VReg& rs1 = rvjit_alloc_reg(blk, inst.rs1, (1ULL << rd.host_reg));
		mov(blk, rd.host_reg, rs1.host_reg);
		rd.dirty = true;
		return;
	}
	VReg& rs1 = rvjit_alloc_reg(
		blk,
		inst.rs1,
		(1ULL << rd.host_reg));

	VReg& rs2 = rvjit_alloc_reg(
		blk,
		inst.rs2,
		(1ULL << rs1.host_reg) | (1ULL << rd.host_reg));
	emit_op(*this, blk, rd, rs1, rs2, pc, tmp);

	rd.dirty = true;
}
inline void JIT_Emitter::inst_emit_i_type(Hart& h, InstructionData& inst, JIT_Block& blk, bool optimize_if_rsz, IOpFunction emit_op, uint64_t pc, void* tmp)
{
	blk.inst_addr_jmp[blk.size] = blk.byte_pos;
	if(inst.rd == 0)
	{
		// x0 write ignored
		return;
	}

	bool rs1_zero = optimize_if_rsz ? (inst.rs1 == 0) : false;
	bool imm_zero = optimize_if_rsz ? (inst.imm == 0) : false;

	VReg& rd = rvjit_alloc_reg(blk, inst.rd, 0);
	if(rs1_zero && imm_zero)
	{
		xor_rr(blk, rd.host_reg, rd.host_reg);
		rd.dirty = true;
		return;
	}

	if(rs1_zero)
	{
		mov_imm32(blk, rd.host_reg, inst.imm);
		rd.dirty = true;
		return;
	}

	if(imm_zero)
	{
		VReg& rs1 = rvjit_alloc_reg(blk, inst.rs1, (1ULL << rd.host_reg));
		mov(blk, rd.host_reg, rs1.host_reg);
		rd.dirty = true;
		return;
	}
	VReg& rs1 = rvjit_alloc_reg(
		blk,
		inst.rs1,
		(1ULL << rd.host_reg));

	emit_op(*this, blk, rd, rs1, inst.imm, pc, tmp);

	rd.dirty = true;
}
inline void JIT_Emitter::inst_emit_s_type(Hart& h, InstructionData& inst, JIT_Block& blk, SOpFunction emit_op, uint64_t pc, void* tmp)
{
	blk.inst_addr_jmp[blk.size] = blk.byte_pos;

	// We don't need any zero optimizations here: we dont have any RD
	VReg& rs1 = rvjit_alloc_reg(
		blk,
		inst.rs1,
		0);

	VReg& rs2 = rvjit_alloc_reg(
		blk,
		inst.rs2,
		(1ULL << rs1.host_reg));
	emit_op(*this, blk, rs1, rs2, inst.imm, pc, tmp);
}
inline void JIT_Emitter::inst_emit_b_type(Hart& h, InstructionData& inst, JIT_Block& blk, BOpFunction emit_op, uint64_t pc, void* tmp)
{
	blk.inst_addr_jmp[blk.size] = blk.byte_pos;

	// We don't need any zero optimizations here: we dont have any RD
	VReg& rs1 = rvjit_alloc_reg(
		blk,
		inst.rs1,
		0);

	VReg& rs2 = rvjit_alloc_reg(
		blk,
		inst.rs2,
		(1ULL << rs1.host_reg));
	emit_op(*this, blk, rs1, rs2, inst.imm, pc, tmp);
}
inline void JIT_Emitter::inst_emit_j_type(Hart& h, InstructionData& inst, JIT_Block& blk, JOpFunction emit_op, uint64_t pc, void* tmp)
{
	blk.inst_addr_jmp[blk.size] = blk.byte_pos;

	VReg& rd = rvjit_alloc_reg(blk, inst.rd, 0);

	VReg& rs1 = rvjit_alloc_reg(
		blk,
		inst.rs1,
		(1ULL << rd.host_reg));

	emit_op(*this, blk, rd, inst.imm, pc, tmp);

	rd.dirty = true;
}
inline void JIT_Emitter::inst_emit_u_type(Hart& h, InstructionData& inst, JIT_Block& blk, UOpFunction emit_op, uint64_t pc, void* tmp)
{
	blk.inst_addr_jmp[blk.size] = blk.byte_pos;
	if(inst.rd == 0)
	{
		// x0 write ignored
		return;
	}

	VReg& rd = rvjit_alloc_reg(blk, inst.rd, 0);

	emit_op(*this, blk, rd, inst.imm, pc, tmp);

	rd.dirty = true;
}

#endif
#endif
