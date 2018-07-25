#include "../detector_if.h"

#include <map>
#include <vector>
#include <atomic>
#include <algorithm>
#include <mutex> // for lock_guard
#include <unordered_map>

#include <iostream>
#include "tsan-if.h"

/*
* Simple mutex implemented as a spinlock
* implements interface of std::mutex
*/
class spinlock {
	std::atomic_flag _flag = ATOMIC_FLAG_INIT;
public:
	inline void lock() noexcept
	{
		while (_flag.test_and_set(std::memory_order_acquire)) {}
	}

	inline bool try_lock() noexcept
	{
		return !(_flag.test_and_set(std::memory_order_acquire));
	}

	inline void unlock() noexcept
	{
		_flag.clear(std::memory_order_release);
	}
};

struct ThreadState {
	void* tsan;
	bool active{ true };
};

/**
 * To avoid false-positives track races only if they are on the heap
 * invert order to get range using lower_bound
 */
static std::atomic<bool>		alloc_readable{ true };
static std::atomic<uint64_t>	misses{ 0 };
static std::atomic<uint64_t>    races{ 0 };
/* Cannot use std::mutex here, hence use spinlock */
static spinlock                 mxspin;
static std::map<uint64_t, size_t, std::greater<uint64_t>> allocations;
static std::vector<int*> stack_trace;
static std::unordered_map<detector::tid_t, ThreadState> thread_states;

struct tsan_params_t {
	bool heap_only{ false };
} params;

void reportRaceCallBack(__tsan_race_info* raceInfo, void * add_race_clb) {
	// Fixes erronous thread exit handling by ignoring races where at least one tid is 0
	if (!raceInfo->access1->user_id || !raceInfo->access2->user_id)
		return;

	detector::Race race;

	// TODO: Currently we assume that there is at most one callback at a time
	__tsan_race_info_access * race_info_ac;
	for (int i = 0; i != 2; ++i) {
		detector::AccessEntry access;
		if (i == 0) {
			race_info_ac = raceInfo->access1;
			races.fetch_add(1, std::memory_order_relaxed);
		}
		else {
			race_info_ac = raceInfo->access2;
		}

		access.thread_id = race_info_ac->user_id;
		access.write = race_info_ac->write;
		access.accessed_memory = race_info_ac->accessed_memory;
		access.access_size = race_info_ac->size;
		access.access_type = race_info_ac->type;

		int ssize = race_info_ac->stack_trace_size;
		access.stack_trace.resize(ssize);
		std::copy((uint64_t*)(race_info_ac->stack_trace),
			((uint64_t*)(race_info_ac->stack_trace)) + ssize,
			access.stack_trace.data());

		uint64_t addr = (uint64_t)(race_info_ac->accessed_memory);
		auto it = allocations.lower_bound(addr);
		if (it != allocations.end() && (addr < (it->first + it->second))) {
			access.onheap = true;
			access.heap_block_begin = (void*)(it->first);
			access.heap_block_size = it->second;
		}

		if (i == 0) {
			race.first = access;
		}
		else {
			race.second = access;
		}
	}
	((void(*)(const detector::Race*))add_race_clb)(&race);
}

/*
 * split address at 32-bit boundary (zero above)
 * TODO: TSAN seems to only support 32 bit addresses!!!
 */
constexpr uint64_t lower_half(uint64_t addr) {
	return (addr & 0x00000000FFFFFFFF);
}

/* Fast trivial hash function using a prime. We expect tids in [1,10^5] */
constexpr uint64_t get_event_id(detector::tid_t parent, detector::tid_t child) {
	return parent * 65521 + child;
}

static void parse_args(int argc, const char ** argv) {
	int processed = 1;
	while (processed < argc) {
		if (strncmp(argv[processed], "--heap-only", 16) == 0) {
			params.heap_only = true;
			++processed;
		}
		else {
			++processed;
		}
	}
}
static void print_config() {
	std::cout << "> Detector Configuration:\n"
		<< "> heap-only: " << (params.heap_only ? "ON" : "OFF") << std::endl;
}

static inline bool on_heap(uint64_t addr) {
	while (!alloc_readable.load(std::memory_order_consume)) {
		auto it = allocations.lower_bound(addr);
		uint64_t beg = it->first;
		size_t   sz = it->second;
		return (addr < (beg + sz));
	}
}

bool detector::init(int argc, const char **argv, Callback rc_clb) {
	parse_args(argc, argv);
	print_config();

	stack_trace.resize(1);
	thread_states.reserve(128);

	__tsan_init_simple(reportRaceCallBack, (void*)rc_clb);
	return true;
}

void detector::finalize() {
	for (auto & t : thread_states) {
		if (t.second.active)
			detector::finish(t.first, t.second.tsan);
	}
	__tsan_fini();
	std::cout << "> ----- SUMMARY -----" << std::endl;
	std::cout << "> Found " << races.load(std::memory_order_relaxed) << " possible data-races" << std::endl;
	std::cout << "> Detector missed " << misses.load() << " possible heap refs" << std::endl;
}

std::string detector::name() {
	return std::string("TSAN");
}

std::string detector::version() {
	return std::string("0.2.0");
}

void detector::acquire(tid_t thread_id, void* mutex, int rec, bool write, bool trylock, tls_t thr) {
	uint64_t addr_32 = lower_half((uint64_t)mutex);

	if (thr == nullptr)
		thr = __tsan_create_thread(thread_id);

	if (write) {
		__tsan_MutexLock(thr, 0, (void*)addr_32, rec, trylock);
	}
	else {
		__tsan_MutexReadLock(thr, 0, (void*)addr_32, trylock);
	}
}

void detector::release(tid_t thread_id, void* mutex, bool write, tls_t thr) {
	uint64_t addr_32 = lower_half((uint64_t)mutex);

	if (thr == nullptr)
		thr = __tsan_create_thread(thread_id);

	if (write) {
		__tsan_MutexUnlock(thr, 0, (void*)addr_32, false);
	}
	else {
		__tsan_MutexReadUnlock(thr, 0, (void*)addr_32);
	}
}

void detector::happens_before(tid_t thread_id, void* identifier) {
	__tsan_happens_before_use_user_tid(thread_id, identifier);
}

void detector::happens_after(tid_t thread_id, void* identifier) {
	__tsan_happens_after_use_user_tid(thread_id, identifier);
}

void detector::read(tid_t thread_id, void* callstack, unsigned stacksize, void* addr, size_t size, tls_t tls) {
	uint64_t addr_32 = lower_half((uint64_t)addr);

	if (!params.heap_only || on_heap((uint64_t)addr_32)) {
		__tsan_read(tls, (void*)addr_32, kSizeLog1, callstack, stacksize);
	}
}

void detector::write(tid_t thread_id, void* callstack, unsigned stacksize, void* addr, size_t size, tls_t tls) {
	uint64_t addr_32 = lower_half((uint64_t)addr);

	if (!params.heap_only || on_heap((uint64_t)addr_32)) {
		__tsan_write(tls, (void*)addr_32, kSizeLog1, callstack, stacksize);
	}
}

void detector::allocate(tid_t thread_id, void* pc, void* addr, size_t size, tls_t tls) {
	uint64_t addr_32 = lower_half((uint64_t)addr);

	if (tls == nullptr)
		tls = __tsan_create_thread(thread_id);

	mxspin.lock();
	alloc_readable.store(false, std::memory_order_release);
	__tsan_malloc(tls, pc, (void*)addr_32, size);
	allocations.emplace((uint64_t)addr_32, size);

	alloc_readable.store(true, std::memory_order_release);
	mxspin.unlock();
}

void detector::deallocate(tid_t thread_id, void* addr, tls_t tls) {
	uint64_t addr_32 = lower_half((uint64_t)addr);
	mxspin.lock();
	alloc_readable.store(false, std::memory_order_release);

	// ocasionally free is called more often than allocate, hence guard
	if (allocations.count(addr_32) == 1) {
		size_t size = allocations[addr_32];
		allocations.erase(addr_32);
		//__tsan_free(addr, size);
		__tsan_reset_range((unsigned int)addr_32, size);
	}

	alloc_readable.store(true, std::memory_order_release);
	mxspin.unlock();
}

void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
	*tls = __tsan_create_thread(child);

	const uint64_t event_id = get_event_id(parent, child);
	std::lock_guard<spinlock> lg(mxspin);
	
	for (auto & t : thread_states) {
		if (t.second.active) {
			__tsan_happens_before_use_user_tid(t.first, (void*)(event_id));
		}
	}
	__tsan_happens_after_use_user_tid(child, (void*)(event_id));
	thread_states[child] = ThreadState{ *tls, true };
}

void detector::join(tid_t parent, tid_t child, tls_t tls) {
	const uint64_t event_id = get_event_id(parent, child);
	__tsan_happens_before_use_user_tid(child, (void*)(event_id));

	std::lock_guard<spinlock> lg(mxspin);
	thread_states[child].active = false;
	for (auto & t : thread_states) {
		if (t.second.active) {
			__tsan_happens_after_use_user_tid(t.first, (void*)(event_id));
		}
	}

	if (tls != nullptr) {
		// we cannot use __tsan_ThreadJoin here, as local tid is not tracked
		// hence use thread finish and draw edge between exited thread
		// and all other threads
		__tsan_ThreadFinish(tls);
	}
}

void detector::detach(tid_t thread_id, tls_t tls) {
	if (tls == nullptr)
		tls = __tsan_create_thread(thread_id);

	__tsan_ThreadDetach(tls, 0, thread_id);
}

void detector::finish(tid_t thread_id, tls_t tls) {
	if (tls != nullptr) {
		__tsan_ThreadFinish(tls);
	}
	std::lock_guard<spinlock> lg(mxspin);
	thread_states[thread_id].active = false;
}