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
#include "timer_st.hpp"
#include <cstdint>

typedef struct
{
	uint64_t timecmp;
	timer_st* timer;
} timecmp_st;

inline uint64_t timecmp_get(timecmp_st* cmp)
{
	return cmp->timecmp;
}
inline void timecmp_set(timecmp_st* cmp, uint64_t timecmp)
{
	cmp->timecmp = timecmp;
}
inline bool timecmp_pending(timecmp_st* cmp)
{
	auto val = timer_get(cmp->timer);
	return val >= timecmp_get(cmp) && val != UINT64_MAX;
}
inline void timecmp_init(timecmp_st* cmp, timer_st* timer)
{
	cmp->timer = timer;
	timecmp_set(cmp, -1);
}
inline uint64_t timecmp_delay(timecmp_st* cmp)
{
	uint64_t timer	 = timer_get(cmp->timer);
	uint64_t timecmp = timecmp_get(cmp);
	return (timer < timecmp) ? (timecmp - timer) : 0;
}
inline uint64_t timecmp_delay_ns(timecmp_st* cmp)
{
	return timer_convert_freq(timecmp_delay(cmp), timer_freq(cmp->timer), 1000000000ULL);
}
