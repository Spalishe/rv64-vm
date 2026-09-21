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
	// Epoch at which compiled code was last installed on a page. A store only
	// invalidates the whole JIT cache when the page still hosts code compiled
	// at the current epoch; otherwise the page is reused data and the stale
	// executed bit is dropped (init pages are freed and rewritten constantly).
	inline std::vector<uint64_t> g_code_page_gen;

	inline void mark_page_executed(uint64_t phys)
	{
		const uint64_t page = phys >> 12;
		if(page >= g_executed_pages.size())
			g_executed_pages.resize(page + 8192, 0);
		g_executed_pages[page] = 1;
	}

	inline void mark_page_code(uint64_t phys)
	{
		const uint64_t page = phys >> 12;
		if(page >= g_code_page_gen.size())
			g_code_page_gen.resize(page + 8192, 0);
		g_code_page_gen[page] = g_smc_epoch.load(std::memory_order_relaxed);
		// Re-arm the W^X detector: a page that hosts current-epoch compiled
		// code must strip W again even if an earlier stale-reuse store had
		// cleared its executed bit, otherwise a later patch to live text
		// would write straight to RAM and skip smc_store_hit.
		if(page >= g_executed_pages.size())
			g_executed_pages.resize(page + 8192, 0);
		g_executed_pages[page] = 1;
	}

	inline void smc_store_hit(uint64_t phys)
	{
		const uint64_t page = phys >> 12;
		if(page < g_executed_pages.size() && g_executed_pages[page])
		{
			const uint64_t cur = g_smc_epoch.load(std::memory_order_relaxed);
			if(page < g_code_page_gen.size() && g_code_page_gen[page] == cur)
				g_smc_epoch.fetch_add(1, std::memory_order_release);
			else
				g_executed_pages[page] = 0; // stale/reused page: no live block
		}
	}

	inline bool was_page_executed(uint64_t phys)
	{
		const uint64_t page = phys >> 12;
		return page < g_executed_pages.size() && g_executed_pages[page] != 0;
	}
}