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

/*
 * x86-64 host code emitter, independent of RISC-V: produces machine code
 * into a CodeBuf. Register pinning / ABI:
 *   RDI = JIT_HartContext* on entry (moved to R12 by the prologue)
 *   R12 = pinned context pointer
 *   R13 = pinned &hart.GPR[0]
 *   RAX,RDX = M-extension multiply/divide accumulator pair (scratch)
 *   RCX = temporary (shift counts, operand copies, divisors)
 *   RSI,RDI,R8-R11 = allocator pool
 * Register numbers are the raw x86-64 encodings (RAX=0 .. R15=15).
 */
#include "rvjit_cfg.hpp"
#include <cstddef>
#include <cstdint>

namespace rv64vm::jit::x86
{
	constexpr uint8_t REG_RAX = 0;
	constexpr uint8_t REG_RCX = 1;
	constexpr uint8_t REG_RDX = 2;
	constexpr uint8_t REG_RBX = 3;
	constexpr uint8_t REG_RSP = 4;
	constexpr uint8_t REG_RBP = 5;
	constexpr uint8_t REG_RSI = 6;
	constexpr uint8_t REG_RDI = 7;
	constexpr uint8_t REG_R8  = 8;
	constexpr uint8_t REG_R9  = 9;
	constexpr uint8_t REG_R10 = 10;
	constexpr uint8_t REG_R11 = 11;
	constexpr uint8_t REG_R12 = 12;
	constexpr uint8_t REG_R13 = 13;

	// Pinned / temporary registers never enter the allocator pool.
	constexpr uint8_t REG_CTX  = REG_R12;
	constexpr uint8_t REG_REGS = REG_R13;
	constexpr uint8_t REG_TMP  = REG_RCX;
	constexpr uint8_t REG_ACC0 = REG_RAX; // mul/div supported dividend / multiplicand
	constexpr uint8_t REG_ACC1 = REG_RDX; // mul/div high word / remainder

	// Allocatable guest-register cache.
	constexpr size_t RVJIT_HOST_REGS						  = 6;
	inline constexpr uint8_t RVJIT_HOST_POOL[RVJIT_HOST_REGS] = {
		REG_RSI, REG_RDI,
		REG_R8, REG_R9, REG_R10, REG_R11
	};

	// Context field offsets the generated code addresses.
	constexpr uint16_t CTX_OFF_REGS	 = 0;
	constexpr uint16_t CTX_OFF_ENTRY = 32;
	constexpr uint16_t CTX_OFF_EXIT	 = 40;

	struct CodeBuf
	{
		uint8_t bytes[RVJIT_FUNC_SIZE];
		uint32_t pos = 0;

		inline void b(uint8_t v)
		{
			if(pos >= RVJIT_FUNC_SIZE) __builtin_trap();
			bytes[pos++] = v;
		}
		inline void dw(uint32_t v)
		{
			b((uint8_t)(v >> 0));
			b((uint8_t)(v >> 8));
			b((uint8_t)(v >> 16));
			b((uint8_t)(v >> 24));
		}
		inline void qw(uint64_t v)
		{
			for(int i = 0; i < 8; i++)
				b((uint8_t)(v >> (i * 8)));
		}
	};

	inline void rex(CodeBuf& cb, bool w, bool r, bool x, bool b)
	{
		cb.b(0x40 | (w ? 8 : 0) | (r ? 4 : 0) | (x ? 2 : 0) | (b ? 1 : 0));
	}

	// mod=11 register/direct operand.
	inline void modrm_reg(CodeBuf& cb, uint8_t regfield, uint8_t rm)
	{
		cb.b(0xC0 | ((regfield & 7) << 3) | (rm & 7));
	}

	// mod=10 memory operand; RSP/R12 need a mandatory SIB.
	inline void modrm_mem(CodeBuf& cb, uint8_t regfield, uint8_t base, int32_t disp)
	{
		cb.b(0x80 | ((regfield & 7) << 3) | (base & 7));
		if((base & 7) == 0x4)
			cb.b(0x24);
		cb.dw((uint32_t)disp);
	}

	inline void mov_rr(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		rex(cb, true, src >= 8, false, dst >= 8);
		cb.b(0x89);
		modrm_reg(cb, src, dst);
	}
	inline void mov_mr(CodeBuf& cb, uint8_t dst, uint8_t base, int32_t disp)
	{
		rex(cb, true, dst >= 8, false, base >= 8);
		cb.b(0x8B);
		modrm_mem(cb, dst, base, disp);
	}
	inline void mov_rm(CodeBuf& cb, uint8_t base, int32_t disp, uint8_t src)
	{
		rex(cb, true, src >= 8, false, base >= 8);
		cb.b(0x89);
		modrm_mem(cb, src, base, disp);
	}
	// mov r64, signext(imm32)
	inline void mov_imm32(CodeBuf& cb, uint8_t dst, int32_t imm)
	{
		rex(cb, true, false, false, dst >= 8);
		cb.b(0xC7);
		modrm_reg(cb, 0, dst);
		cb.dw((uint32_t)imm);
	}
	inline void mov_imm64(CodeBuf& cb, uint8_t dst, uint64_t imm)
	{
		rex(cb, true, false, false, dst >= 8);
		cb.b(0xB8 | (dst & 7));
		cb.qw(imm);
	}
	inline void movzx_r64_r8(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		rex(cb, true, dst >= 8, false, src >= 8);
		cb.b(0x0F);
		cb.b(0xB6);
		modrm_reg(cb, dst, src);
	}
	// REX grants SIL/DIL/R8L..R11L access.
	inline void movzx_ecx_r8(CodeBuf& cb, uint8_t src)
	{
		rex(cb, false, false, false, src >= 8);
		cb.b(0x0F);
		cb.b(0xB6);
		modrm_reg(cb, REG_RCX, src);
	}
	inline void movsxd(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		rex(cb, true, dst >= 8, false, src >= 8);
		cb.b(0x63);
		modrm_reg(cb, dst, src);
	}
	inline void push_r(CodeBuf& cb, uint8_t r)
	{
		if(r >= 8) rex(cb, false, false, false, true);
		cb.b(0x50 | (r & 7));
	}
	inline void pop_r(CodeBuf& cb, uint8_t r)
	{
		if(r >= 8) rex(cb, false, false, false, true);
		cb.b(0x58 | (r & 7));
	}
	inline void ret(CodeBuf& cb)
	{
		cb.b(0xC3);
	}

	// Arithmetic on a destination register; regfield: 0=add 1=or 4=and 5=sub 6=xor 7=cmp.
	inline void arith_rr64(CodeBuf& cb, uint8_t regfield, uint8_t dst, uint8_t src)
	{
		rex(cb, true, src >= 8, false, dst >= 8);
		cb.b(0x01 | (static_cast<uint8_t>(regfield) << 3));
		modrm_reg(cb, src, dst);
	}
	inline void arith_rr32(CodeBuf& cb, uint8_t regfield, uint8_t dst, uint8_t src)
	{
		rex(cb, false, src >= 8, false, dst >= 8);
		cb.b(0x01 | (static_cast<uint8_t>(regfield) << 3));
		modrm_reg(cb, src, dst);
	}
	inline void arith_imm64(CodeBuf& cb, uint8_t regfield, uint8_t dst, int32_t imm)
	{
		rex(cb, true, false, false, dst >= 8);
		cb.b(0x81);
		modrm_reg(cb, regfield, dst);
		cb.dw((uint32_t)imm);
	}
	inline void arith_imm32(CodeBuf& cb, uint8_t regfield, uint8_t dst, int32_t imm)
	{
		rex(cb, false, false, false, dst >= 8);
		cb.b(0x81);
		modrm_reg(cb, regfield, dst);
		cb.dw((uint32_t)imm);
	}
	inline void add_rr(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr64(cb, 0, dst, src);
	}
	inline void or_rr(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr64(cb, 1, dst, src);
	}
	inline void and_rr(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr64(cb, 4, dst, src);
	}
	inline void sub_rr(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr64(cb, 5, dst, src);
	}
	inline void xor_rr(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr64(cb, 6, dst, src);
	}
	inline void cmp_rr(CodeBuf& cb, uint8_t a, uint8_t b)
	{
		arith_rr64(cb, 7, a, b);
	}
	inline void add_rr32(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr32(cb, 0, dst, src);
	}
	inline void or_rr32(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr32(cb, 1, dst, src);
	}
	inline void and_rr32(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr32(cb, 4, dst, src);
	}
	inline void sub_rr32(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr32(cb, 5, dst, src);
	}
	inline void xor_rr32(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		arith_rr32(cb, 6, dst, src);
	}
	inline void add_imm(CodeBuf& cb, uint8_t dst, int32_t imm)
	{
		arith_imm64(cb, 0, dst, imm);
	}
	inline void or_imm(CodeBuf& cb, uint8_t dst, int32_t imm)
	{
		arith_imm64(cb, 1, dst, imm);
	}
	inline void and_imm(CodeBuf& cb, uint8_t dst, int32_t imm)
	{
		arith_imm64(cb, 4, dst, imm);
	}
	inline void xor_imm(CodeBuf& cb, uint8_t dst, int32_t imm)
	{
		arith_imm64(cb, 6, dst, imm);
	}
	inline void cmp_imm(CodeBuf& cb, uint8_t dst, int32_t imm)
	{
		arith_imm64(cb, 7, dst, imm);
	}
	inline void add_imm32(CodeBuf& cb, uint8_t dst, int32_t imm)
	{
		arith_imm32(cb, 0, dst, imm);
	}

	inline void neg64(CodeBuf& cb, uint8_t dst)
	{
		rex(cb, true, false, false, dst >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 3, dst);
	}
	inline void neg32(CodeBuf& cb, uint8_t dst)
	{
		rex(cb, false, false, false, dst >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 3, dst);
	}

	// Shifts; regfield: 4=shl 5=shr 7=sar.
	inline void shift_r64_imm(CodeBuf& cb, uint8_t regfield, uint8_t dst, uint8_t imm)
	{
		rex(cb, true, false, false, dst >= 8);
		cb.b(0xC1);
		modrm_reg(cb, regfield, dst);
		cb.b(imm);
	}
	inline void shift_r32_imm(CodeBuf& cb, uint8_t regfield, uint8_t dst, uint8_t imm)
	{
		rex(cb, false, false, false, dst >= 8);
		cb.b(0xC1);
		modrm_reg(cb, regfield, dst);
		cb.b(imm);
	}
	inline void shift_r64_cl(CodeBuf& cb, uint8_t regfield, uint8_t dst)
	{
		rex(cb, true, false, false, dst >= 8);
		cb.b(0xD3);
		modrm_reg(cb, regfield, dst);
	}
	inline void shift_r32_cl(CodeBuf& cb, uint8_t regfield, uint8_t dst)
	{
		rex(cb, false, false, false, dst >= 8);
		cb.b(0xD3);
		modrm_reg(cb, regfield, dst);
	}

	// Set-on-condition; writes dst's low byte. cc: 0x92=setb 0x9C=setl 0x94=sete.
	// A REX prefix is always emitted (mandatory for SIL/DIL/R8L..R11L).
	inline void setcc(CodeBuf& cb, uint8_t cc, uint8_t dst)
	{
		rex(cb, false, false, false, dst >= 8);
		cb.b(0x0F);
		cb.b(cc);
		modrm_reg(cb, 0, dst);
	}

	// mov r32, r32 (zero-extends to r64)
	inline void mov_rr32(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		rex(cb, false, src >= 8, false, dst >= 8);
		cb.b(0x89);
		modrm_reg(cb, src, dst);
	}

	// MUL: RDX:RAX = RAX * rm (unsigned 128-bit)
	inline void mul_r(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, true, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 4, rm);
	}
	inline void mul_r32(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, false, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 4, rm);
	}
	// IMUL: RDX:RAX = RAX * rm (signed 128-bit)
	inline void imul_r(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, true, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 5, rm);
	}
	inline void imul_r32(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, false, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 5, rm);
	}
	// imul dst, src: dst = dst * src (low half)
	inline void imul_rr(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		rex(cb, true, dst >= 8, false, src >= 8);
		cb.b(0x0F);
		cb.b(0xAF);
		modrm_reg(cb, dst, src);
	}
	inline void imul_rr32(CodeBuf& cb, uint8_t dst, uint8_t src)
	{
		rex(cb, false, dst >= 8, false, src >= 8);
		cb.b(0x0F);
		cb.b(0xAF);
		modrm_reg(cb, dst, src);
	}
	// DIV: RAX = RDX:RAX / rm, RDX = remainder (unsigned). Signed = IDIV (/7).
	inline void div_r(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, true, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 6, rm);
	}
	inline void div_r32(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, false, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 6, rm);
	}
	inline void idiv_r(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, true, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 7, rm);
	}
	inline void idiv_r32(CodeBuf& cb, uint8_t rm)
	{
		rex(cb, false, false, false, rm >= 8);
		cb.b(0xF7);
		modrm_reg(cb, 7, rm);
	}
	// Sign-extend RAX into RDX: cqo = 64-bit, cdq = 32-bit.
	inline void cqo(CodeBuf& cb)
	{
		rex(cb, true, false, false, false);
		cb.b(0x99);
	}
	inline void cdq(CodeBuf& cb)
	{
		cb.b(0x99);
	}

	// rel8 conditional jump / jump; returns the index of the rel8 byte so the
	// caller can patch it (see patch_rel8) once the target position is known.
	inline uint32_t jcc8(CodeBuf& cb, uint8_t opcode)
	{
		cb.b(opcode);
		const uint32_t rel = cb.pos;
		cb.b(0);
		return rel;
	}
	inline void patch_rel8(CodeBuf& cb, uint32_t rel, uint32_t target)
	{
		cb.bytes[rel] = (uint8_t)(target - (rel + 1));
	}
}