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
#include <chrono>
#include <cstdint>

typedef struct
{
	uint64_t begin;
	uint64_t freq;
} timer_st;

inline uint64_t timer_clocksource(uint64_t freq)
{
	auto now	  = std::chrono::steady_clock::now();
	auto duration = now.time_since_epoch();
	uint64_t ns	  = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
	return (ns * freq) / 1000000000ULL;
}
inline uint64_t timer_freq(timer_st* timer)
{
	return timer->freq;
}
inline uint64_t timer_get(timer_st* timer)
{
	return timer_clocksource(timer->freq) - timer->begin;
}
inline void timer_set(timer_st* timer, uint64_t time)
{
	timer->begin = timer_clocksource(timer->freq) - time;
}
inline void timer_init(timer_st* timer, uint64_t freq)
{
	timer->freq = freq;
	timer_set(timer, 0);
}
inline uint64_t timer_convert_freq(uint64_t clk, uint64_t src_freq, uint64_t dst_freq)
{
	uint64_t clk_div = clk / src_freq;
	uint64_t clk_rem = clk - (clk_div * src_freq);
	return (clk_div * dst_freq) + (clk_rem * dst_freq / src_freq);
}
