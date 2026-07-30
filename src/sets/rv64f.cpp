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

#ifdef USE_FPU
#include "../../include/decode.hpp"
#include "../../include/hart.hpp"
#include <cfenv>
#include <cmath>

using namespace rv64vm::runner;

inline double rm_func(double val, uint8_t rm, Hart& hart, bool& is_error)
{
	uint8_t cmp = 0;
	if(rm == 0b111) [[likely]]
		cmp = hart.fcsr.fields.frm;
	else
	{
		cmp = rm;
	}
	if(cmp == 0b101 || cmp == 0b110)
	{
		// Reserved
		is_error = true;
		return val;
	}
	if(cmp == 0b100)
	{
		return std::round(val);
	}
	else
	{
		switch(cmp)
		{
			case 0b000:
				std::fesetround(FE_TONEAREST);
				break; // RNE
			case 0b001:
				std::fesetround(FE_TOWARDZERO);
				break; // RTZ
			case 0b010:
				std::fesetround(FE_DOWNWARD);
				break; // RDN
			case 0b011:
				std::fesetround(FE_UPWARD);
				break; // RUP
		}
		return val;
	}
	return val;
}
inline float f32_in(double reg)
{
	uint32_t bits = static_cast<uint32_t>(std::bit_cast<uint64_t>(reg));
	return std::bit_cast<float>(bits);
}

inline double f32_out(float val)
{
	uint64_t bits = 0xFFFFFFFF00000000ULL | std::bit_cast<uint32_t>(val);
	return std::bit_cast<double>(bits);
}

inline float cast_float(Hart& hart, double val)
{
	if(std::isnan(val)) [[unlikely]]
	{
		hart.fcsr.fields.fflags |= (1 << 4);
		return std::bit_cast<float>(0x7FC00000U);
	}
	else [[likely]]
		return static_cast<float>(val);
	return (float)val;
}
inline bool issnan(float val)
{
	uint32_t bits = std::bit_cast<uint32_t>(val);
	bool is_nan	  = ((bits & 0x7F800000) == 0x7F800000) && ((bits & 0x007FFFFF) != 0);
	return is_nan && ((bits & 0x00400000) == 0);
}
/*inline bool issnan(double val) {
	uint64_t bits = std::bit_cast<uint64_t>(val);
	bool is_nan = ((bits & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) && ((bits & 0x000FFFFFFFFFFFFFULL) != 0);
	return is_nan && ((bits & 0x0008000000000000ULL) == 0);
}*/
// for rv64d
template <typename T, int64_t MinValue, uint64_t MaxValue>
inline int64_t finalize_fcvt(float src, double exact, int64_t rounded, Hart& hart)
{
	if(std::isnan(src)) [[unlikely]]
	{
		hart.fcsr.fields.fflags = (hart.fcsr.fields.fflags | (1 << 4)) & ~(1 << 0);
		return static_cast<int64_t>(MaxValue);
	}

	if constexpr(std::is_unsigned_v<T>)
	{
		if(std::isinf(src) && !std::signbit(src) || exact >= (static_cast<double>(MaxValue) + 1.0))
		{
			hart.fcsr.fields.fflags = (hart.fcsr.fields.fflags | (1 << 4)) & ~(1 << 0);
			return static_cast<int64_t>(MaxValue);
		}
		if(exact <= -1.0 || (rounded < 0 && rounded != INT64_MIN))
		{
			hart.fcsr.fields.fflags = (hart.fcsr.fields.fflags | (1 << 4)) & ~(1 << 0);
			return 0;
		}
	}
	else
	{
		if((std::isinf(src) && std::signbit(src)) || exact < static_cast<double>(MinValue))
		{
			hart.fcsr.fields.fflags = (hart.fcsr.fields.fflags | (1 << 4)) & ~(1 << 0);
			return MinValue;
		}
		if((std::isinf(src) && !std::signbit(src)) || exact >= (static_cast<double>(MaxValue) + 1.0))
		{
			hart.fcsr.fields.fflags = (hart.fcsr.fields.fflags | (1 << 4)) & ~(1 << 0);
			return static_cast<int64_t>(MaxValue);
		}
	}

	if(static_cast<double>(rounded) != exact)
	{
		hart.fcsr.fields.fflags |= (1 << 0);
	}

	return rounded;
}

#define RM_HANDLE(val, inst, rm, hart)                                    \
	{                                                                     \
		do                                                                \
		{                                                                 \
			bool error_flag = false;                                      \
			double inter	= rm_func((val), (rm), (hart), (error_flag)); \
			if(error_flag)                                                \
			{                                                             \
				return { false, false, 4, 2, inst };                      \
			}                                                             \
			(val) = static_cast<float>(inter);                            \
		} while(0);                                                       \
	}

#define FLOAT_START std::feclearexcept(FE_ALL_EXCEPT);
#define FLOAT_END(fflags)                                        \
	{                                                            \
		do                                                       \
		{                                                        \
			int host_except = std::fetestexcept(FE_ALL_EXCEPT);  \
			if(host_except & FE_INVALID) (fflags) |= (1 << 4);   \
			if(host_except & FE_DIVBYZERO) (fflags) |= (1 << 3); \
			if(host_except & FE_OVERFLOW) (fflags) |= (1 << 2);  \
			if(host_except & FE_UNDERFLOW) (fflags) |= (1 << 1); \
			if(host_except & FE_INEXACT) (fflags) |= (1 << 0);   \
		} while(0);                                              \
	}

ExecReturn exec_FMADD_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	float s3   = f32_in(hart.FPR[inst.rs3]);
	double res = (double)s1 * (double)s2 + (double)s3;
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FMSUB_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	float s3   = f32_in(hart.FPR[inst.rs3]);
	double res = (double)s1 * (double)s2 - (double)s3;
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FNMADD_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	float s3   = f32_in(hart.FPR[inst.rs3]);
	double res = -(double)s1 * (double)s2 - (double)s3;

	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FNMSUB_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	float s3   = f32_in(hart.FPR[inst.rs3]);
	double res = -(double)s1 * (double)s2 + (double)s3;
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FADD_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	double res = (double)s1 + (double)s2;
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FSUB_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	double res = (double)s1 - (double)s2;
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FMUL_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	double res = (double)s1 * (double)s2;
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FDIV_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	float s2   = f32_in(hart.FPR[inst.rs2]);
	double res = (double)s1 / (double)s2;
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FSQRT_S(Hart& hart, InstructionData& inst)
{
	float s1   = f32_in(hart.FPR[inst.rs1]);
	double res = (double)sqrt(s1);
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FSGNJ_S(Hart& hart, InstructionData& inst)
{
	float f_src1	  = f32_in(hart.FPR[inst.rs1]);
	float f_src2	  = f32_in(hart.FPR[inst.rs2]);
	float res		  = std::copysign(f_src1, f_src2);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FSGNJN_S(Hart& hart, InstructionData& inst)
{
	float f_src1	  = f32_in(hart.FPR[inst.rs1]);
	float f_src2	  = f32_in(hart.FPR[inst.rs2]);
	float res		  = std::copysign(f_src1, -f_src2);
	hart.FPR[inst.rd] = f32_out(cast_float(hart, res));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FSGNJX_S(Hart& hart, InstructionData& inst)
{
	uint32_t bits1				 = std::bit_cast<uint32_t>(f32_in(hart.FPR[inst.rs1]));
	uint32_t bits2				 = std::bit_cast<uint32_t>(f32_in(hart.FPR[inst.rs2]));
	constexpr uint32_t sign_mask = 0x80000000;
	uint32_t res_bits			 = (bits1 & ~sign_mask) | ((bits1 ^ bits2) & sign_mask);
	hart.FPR[inst.rd]			 = f32_out(std::bit_cast<float>(res_bits));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FMIN_S(Hart& hart, InstructionData& inst)
{
	float s1 = f32_in(hart.FPR[inst.rs1]);
	float s2 = f32_in(hart.FPR[inst.rs2]);

	bool nan1 = std::isnan(s1);
	bool nan2 = std::isnan(s2);

	if((nan1 && issnan(s1)) || (nan2 && issnan(s2))) [[unlikely]]
	{
		hart.fcsr.fields.fflags |= (1 << 4); // NV флаг (16)
	}

	float res;

	if(nan1 && nan2) [[unlikely]]
	{
		res = std::bit_cast<float>(0x7FC00000U);
	}
	else if(nan1)
	{
		res = s2;
	}
	else if(nan2)
	{
		res = s1;
	}
	else [[likely]]
	{
		if(s1 == 0.0f && s2 == 0.0f)
		{
			res = std::signbit(s1) ? s1 : s2;
		}
		else
		{
			res = std::isless(s1, s2) ? s1 : s2;
		}
	}

	hart.FPR[inst.rd] = f32_out(res);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FMAX_S(Hart& hart, InstructionData& inst)
{
	float s1 = f32_in(hart.FPR[inst.rs1]);
	float s2 = f32_in(hart.FPR[inst.rs2]);

	bool nan1 = std::isnan(s1);
	bool nan2 = std::isnan(s2);

	if((nan1 && issnan(s1)) || (nan2 && issnan(s2))) [[unlikely]]
	{
		hart.fcsr.fields.fflags |= (1 << 4);
	}

	float res;

	if(nan1 && nan2) [[unlikely]]
	{
		res = std::bit_cast<float>(0x7FC00000U);
	}
	else if(nan1)
	{
		res = s2;
	}
	else if(nan2)
	{
		res = s1;
	}
	else [[likely]]
	{
		if(s1 == 0.0f && s2 == 0.0f)
		{
			res = !std::signbit(s1) ? s1 : s2;
		}
		else
		{
			res = std::isgreater(s1, s2) ? s1 : s2;
		}
	}

	hart.FPR[inst.rd] = f32_out(res);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCVT_W_S(Hart& hart, InstructionData& inst)
{
	float src	 = f32_in(hart.FPR[inst.rs1]);
	double exact = static_cast<double>(src);

	bool is_error		= false;
	double intermediate = rm_func(exact, inst.rm, hart, is_error);
	if(is_error) [[unlikely]]
		return { false, false, 4, 2, inst.inst };

	int64_t rounded = std::llrint(intermediate);

	int64_t res		  = finalize_fcvt<int32_t, INT32_MIN, INT32_MAX>(src, exact, rounded, hart);
	hart.GPR[inst.rd] = static_cast<int32_t>(res);

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCVT_WU_S(Hart& hart, InstructionData& inst)
{
	float src	 = f32_in(hart.FPR[inst.rs1]);
	double exact = static_cast<double>(src);

	bool is_error		= false;
	double intermediate = rm_func(exact, inst.rm, hart, is_error);
	if(is_error) [[unlikely]]
		return { false, false, 4, 2, inst.inst };

	int64_t rounded = std::llrint(intermediate);

	int64_t res		  = finalize_fcvt<uint32_t, 0, UINT32_MAX>(src, exact, rounded, hart);
	hart.GPR[inst.rd] = static_cast<int32_t>(static_cast<uint32_t>(res));

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCVT_S_W(Hart& hart, InstructionData& inst)
{
	int32_t src	 = static_cast<int32_t>(hart.GPR[inst.rs1]);
	double exact = static_cast<double>(src);

	FLOAT_START
	RM_HANDLE(exact, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(static_cast<float>(exact));

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCVT_S_WU(Hart& hart, InstructionData& inst)
{
	uint32_t src = static_cast<uint32_t>(hart.GPR[inst.rs1]);
	double exact = static_cast<double>(src);

	FLOAT_START
	RM_HANDLE(exact, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(static_cast<float>(exact));

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCVT_L_S(Hart& hart, InstructionData& inst)
{
	float src	 = f32_in(hart.FPR[inst.rs1]);
	double exact = static_cast<double>(src);

	bool is_error		= false;
	double intermediate = rm_func(exact, inst.rm, hart, is_error);
	if(is_error) [[unlikely]]
		return { false, false, 4, 2, inst.inst };

	int64_t rounded = std::llrint(intermediate);

	hart.GPR[inst.rd] = finalize_fcvt<int64_t, INT64_MIN, INT64_MAX>(src, exact, rounded, hart);

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCVT_LU_S(Hart& hart, InstructionData& inst)
{
	float src	 = f32_in(hart.FPR[inst.rs1]);
	double exact = static_cast<double>(src);

	bool is_error		= false;
	double intermediate = rm_func(exact, inst.rm, hart, is_error);
	if(is_error) [[unlikely]]
		return { false, false, 4, 2, inst.inst };

	int64_t rounded = std::llrint(intermediate);

	uint64_t res	  = static_cast<uint64_t>(finalize_fcvt<uint64_t, 0, UINT64_MAX>(src, exact, rounded, hart));
	hart.GPR[inst.rd] = res;

	return { true, false, 4, 0, 0 };
}

ExecReturn exec_FCVT_S_L(Hart& hart, InstructionData& inst)
{
	int64_t src	 = static_cast<int64_t>(hart.GPR[inst.rs1]);
	double exact = static_cast<double>(src);

	FLOAT_START
	RM_HANDLE(exact, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);

	hart.FPR[inst.rd] = f32_out(static_cast<float>(exact));

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCVT_S_LU(Hart& hart, InstructionData& inst)
{
	uint64_t src = static_cast<uint64_t>(hart.GPR[inst.rs1]);
	double exact = static_cast<double>(src);

	FLOAT_START
	RM_HANDLE(exact, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);

	hart.FPR[inst.rd] = f32_out(static_cast<float>(exact));

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FMV_X_W(Hart& hart, InstructionData& inst)
{
	float src		  = f32_in(hart.FPR[inst.rs1]);
	hart.GPR[inst.rd] = static_cast<int32_t>(std::bit_cast<uint32_t>(src));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FMV_W_X(Hart& hart, InstructionData& inst)
{
	uint32_t src_bits = static_cast<uint32_t>(hart.GPR[inst.rs1]);
	hart.FPR[inst.rd] = f32_out(std::bit_cast<float>(src_bits));

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FEQ_S(Hart& hart, InstructionData& inst)
{
	float src1 = f32_in(hart.FPR[inst.rs1]);
	float src2 = f32_in(hart.FPR[inst.rs2]);

	if(issnan(src1) || issnan(src2)) [[unlikely]]
	{
		hart.fcsr.fields.fflags |= (1 << 4); // NV
		hart.GPR[inst.rd] = 0;
	}
	else
	{
		hart.GPR[inst.rd] = (src1 == src2) ? 1 : 0;
	}

	return { true, false, 4, 0, 0 };
}

ExecReturn exec_FLT_S(Hart& hart, InstructionData& inst)
{
	float src1 = f32_in(hart.FPR[inst.rs1]);
	float src2 = f32_in(hart.FPR[inst.rs2]);

	if(std::isnan(src1) || std::isnan(src2)) [[unlikely]]
	{
		hart.fcsr.fields.fflags |= (1 << 4); // NV
		hart.GPR[inst.rd] = 0;
	}
	else
	{
		// isless somehow faster than < operator
		hart.GPR[inst.rd] = std::isless(src1, src2) ? 1 : 0;
	}

	return { true, false, 4, 0, 0 };
}

ExecReturn exec_FLE_S(Hart& hart, InstructionData& inst)
{
	float src1 = f32_in(hart.FPR[inst.rs1]);
	float src2 = f32_in(hart.FPR[inst.rs2]);

	if(std::isnan(src1) || std::isnan(src2)) [[unlikely]]
	{
		hart.fcsr.fields.fflags |= (1 << 4); // NV
		hart.GPR[inst.rd] = 0;
	}
	else
	{
		// same applies to islessequal
		hart.GPR[inst.rd] = std::islessequal(src1, src2) ? 1 : 0;
	}

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FCLASS_S(Hart& hart, InstructionData& inst)
{
	float src = f32_in(hart.FPR[inst.rs1]);

	// if num is < 0, then true
	bool sign = std::signbit(src);
	int type  = std::fpclassify(src);

	uint64_t mask = 0;

	switch(type)
	{
		case FP_INFINITE:
			mask = sign ? (1 << 0) : (1 << 7); // -inf or +inf
			break;

		case FP_ZERO:
			mask = sign ? (1 << 3) : (1 << 4); // -0.0 or +0.0
			break;

		case FP_SUBNORMAL:
			mask = sign ? (1 << 2) : (1 << 5); // negative/positive subnormal
			break;

		case FP_NAN:
		{
			// if 22 bit is 1: its quite nan
			uint32_t bits = std::bit_cast<uint32_t>(src);
			bool is_quiet = (bits & 0x00400000) != 0;
			mask		  = is_quiet ? (1 << 9) : (1 << 8); // qNaN or sNaN
			break;
		}

		case FP_NORMAL:
		default:
			mask = sign ? (1 << 1) : (1 << 6); // negative/positive normal
			break;
	}

	hart.GPR[inst.rd] = mask;

	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FLW(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint32_t val;
	MemoryReturn success = hart.get_mmio()->read(hart, addr, MemorySize::Int, &val);

	if(success.is_success)
	{
		hart.FPR[inst.rd] = f32_out(std::bit_cast<float>(val));
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}

ExecReturn exec_FSW(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	float f_val		 = f32_in(hart.FPR[inst.rs2]);
	uint32_t val	 = std::bit_cast<uint32_t>(f_val);
	MemoryReturn out = hart.get_mmio()->write(hart, addr, MemorySize::Int, val);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}

void InstructionDecoder::init_rv64f()
{
	register_instr("*****00******************1000011", exec_FMADD_S);
	register_instr("*****00******************1000111", exec_FMSUB_S);
	register_instr("*****00******************1001111", exec_FNMADD_S);
	register_instr("*****00******************1001011", exec_FNMSUB_S);
	register_instr("0000000******************1010011", exec_FADD_S);
	register_instr("0000100******************1010011", exec_FSUB_S);
	register_instr("0001000******************1010011", exec_FMUL_S);
	register_instr("0001100******************1010011", exec_FDIV_S);
	register_instr("010110000000*************1010011", exec_FSQRT_S);
	register_instr("0010000**********000*****1010011", exec_FSGNJ_S);
	register_instr("0010000**********001*****1010011", exec_FSGNJN_S);
	register_instr("0010000**********010*****1010011", exec_FSGNJX_S);
	register_instr("0010100**********000*****1010011", exec_FMIN_S);
	register_instr("0010100**********001*****1010011", exec_FMAX_S);
	register_instr("110000000000*************1010011", exec_FCVT_W_S);
	register_instr("110000000001*************1010011", exec_FCVT_WU_S);
	register_instr("110100000000*************1010011", exec_FCVT_S_W);
	register_instr("110100000001*************1010011", exec_FCVT_S_WU);
	register_instr("110000000010*************1010011", exec_FCVT_L_S);
	register_instr("110000000011*************1010011", exec_FCVT_LU_S);
	register_instr("110100000010*************1010011", exec_FCVT_S_L);
	register_instr("110100000011*************1010011", exec_FCVT_S_LU);
	register_instr("111000000000*****000*****1010011", exec_FMV_X_W);
	register_instr("111100000000*****000*****1010011", exec_FMV_W_X);
	register_instr("1010000**********010*****1010011", exec_FEQ_S);
	register_instr("1010000**********001*****1010011", exec_FLT_S);
	register_instr("1010000**********000*****1010011", exec_FLE_S);
	register_instr("111000000000*****001*****1010011", exec_FCLASS_S);
	register_instr("*****************010*****0000111", exec_FLW, imm_I);
	register_instr("*****************010*****0100111", exec_FSW, imm_S);
}
#endif
