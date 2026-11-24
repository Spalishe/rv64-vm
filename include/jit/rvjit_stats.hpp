#pragma once
#include <atomic>
#include <cstdio>

#ifdef RVJIT_STATS
#define RVJIT_STAT(name) ::rv64vm::jit::stats::name.fetch_add(1, std::memory_order_relaxed)
namespace rv64vm::jit::stats
{
	inline std::atomic<uint64_t> jit_calls{ 0 };
	inline std::atomic<uint64_t> jit_zero{ 0 };
	inline std::atomic<uint64_t> memo_hits{ 0 };
	inline std::atomic<uint64_t> interp{ 0 };
	inline std::atomic<uint64_t> lookup{ 0 };
	inline std::atomic<uint64_t> lookup_hits{ 0 };
	inline std::atomic<uint64_t> compile{ 0 };
	inline std::atomic<uint64_t> salvage{ 0 };
	inline std::atomic<uint64_t> giveup{ 0 };
	inline std::atomic<uint64_t> seq_skip{ 0 };
	inline std::atomic<uint64_t> satp_inval{ 0 };
	inline std::atomic<uint64_t> sfence_inval{ 0 };
	inline std::atomic<uint64_t> smc_bump{ 0 };
	inline std::atomic<uint64_t> tlb_flush{ 0 };
	inline std::atomic<uint64_t> note_exec_strip{ 0 };

	inline void dump()
	{
		std::fprintf(stderr,
					 "RVJIT_STATS jit_calls=%llu jit_zero=%llu memo_hits=%llu interp=%llu "
					 "lookup=%llu lookup_hits=%llu compile=%llu salvage=%llu giveup=%llu "
					 "seq_skip=%llu satp_inval=%llu sfence_inval=%llu smc_bump=%llu tlb_flush=%llu note_exec_strip=%llu\n",
					 (unsigned long long)jit_calls.load(), (unsigned long long)jit_zero.load(),
					 (unsigned long long)memo_hits.load(), (unsigned long long)interp.load(),
					 (unsigned long long)lookup.load(), (unsigned long long)lookup_hits.load(),
					 (unsigned long long)compile.load(), (unsigned long long)salvage.load(),
					 (unsigned long long)giveup.load(), (unsigned long long)seq_skip.load(),
					 (unsigned long long)satp_inval.load(), (unsigned long long)sfence_inval.load(),
					 (unsigned long long)smc_bump.load(), (unsigned long long)tlb_flush.load(),
					 (unsigned long long)note_exec_strip.load());
	}
}
#else
#define RVJIT_STAT(name) ((void)0)
namespace rv64vm::jit::stats
{
	inline void dump() {}
}
#endif
