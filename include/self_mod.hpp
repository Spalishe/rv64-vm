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
#include <atomic>
#include <cstdint>
#include <vector>

// Self-modifying-code tracking: the guest may rewrite its text without
// FENCE.I (kernel decompression / runtime patching). A store to a page that
// was ever compiled bumps a global epoch; caches validate entries against it.
namespace rv64vm
{
	inline std::atomic<uint64_t> g_smc_epoch{ 0 };
	inline std::vector<uint8_t> g_executed_pages;

	inline void mark_page_executed(uint64_t phys)
	{
		const uint64_t page = phys >> 12;
		if(page >= g_executed_pages.size())
			g_executed_pages.resize(page + 8192, 0);
		g_executed_pages[page] = 1;
	}

	inline void smc_store_hit(uint64_t phys)
	{
		const uint64_t page = phys >> 12;
		if(page < g_executed_pages.size() && g_executed_pages[page])
			g_smc_epoch.fetch_add(1);
	}
}