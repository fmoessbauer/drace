#include "../detector_if.h"

#include <map>
#include <vector>
#include <atomic>

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

/**
 * To avoid false-positives track races only if they are on the heap
 * invert order to get range using lower_bound
 */
// TODO: Make accesses thread safe using memory_order acquire release
// Or use epoches
static std::atomic<bool>		alloc_readable{ true };
static std::atomic<uint64_t>	misses{ 0 };
static std::atomic<uint64_t>	races{ 0 };
/* Cannot use std::mutex here, hence use spinlock */
static spinlock                 mxspin;
static std::map<uint64_t, size_t, std::greater<uint64_t>> allocations;
static std::vector<int*> stack_trace;

struct tsan_params_t {
	bool heap_only{ false };
} params;

void reportRaceCallBack(__tsan_race_info* raceInfo, void* stack_demangler) {
	// Fixes erronous thread exit handling by ignoring races where at least one tid is 0
	if (!raceInfo->access1->user_id || !raceInfo->access2->user_id)
		return;

	// TODO: Currently we assume that there is at most one callback at a time
	std::cout << "----- DATA Race -----" << std::endl;
	for (int i = 0; i != 2; ++i) {
		__tsan_race_info_access* race_info_ac = NULL;

		if (i == 0) {
			race_info_ac = raceInfo->access1;
			std::cout << "Access 1 tid: " << race_info_ac->user_id << " ";
			++races;
		}
		else {
			race_info_ac = raceInfo->access2;
			std::cout << "Access 2 tid: " << race_info_ac->user_id << " ";
		}
		std::cout << (race_info_ac->write == 1 ? "write" : "read") << " to/from " << race_info_ac->accessed_memory << " with size " << race_info_ac->size << ". Stack(Size " << race_info_ac->stack_trace_size << ")" << "Type: " << race_info_ac->type << " :" << ::std::endl;
		// DEBUG: Show HEAP Block if any
		// Spinlock sometimes dead-locks here
		//mxspin.lock();
		uint64_t addr = (uint64_t)(race_info_ac->accessed_memory);
		auto it = allocations.lower_bound(addr);
		if (it != allocations.end() && (addr < (it->first + it->second))) {
			printf("Block begin at %p, size %d\n", it->first, it->second);
		}
		else {
			printf("Block not on heap (anymore)\n");
		}
		//mxspin.unlock();

		// demagle stack
		if (stack_demangler != nullptr) {
			// Type of stack_demangler: (void*) -> void
			((void(*)(void*))stack_demangler)(race_info_ac->stack_trace);
		}
	}
	std::cout << "----- --------- -----" << std::endl;
}

/*
 * split address at 32-bit boundary (zero above)
 * TODO: TSAN seems to only support 32 bit addresses!!!
 */
inline uint64_t lower_half(uint64_t addr) {
	return ((uint64_t)(addr) & 0x00000000FFFFFFFF);
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
	while(!alloc_readable.load(std::memory_order_consume)) {
		auto it = allocations.lower_bound(addr);
		uint64_t beg = it->first;
		size_t   sz = it->second;
		return (addr < (beg + sz));
	}
}

bool detector::init(int argc, const char **argv, void(*stack_demangler)(void*)) {
	parse_args(argc, argv);
	print_config();

	stack_trace.resize(1);

	__tsan_init_simple(reportRaceCallBack, (void*)stack_demangler);
	return true;
}

void detector::finalize() {
	__tsan_fini();
	std::cout << "> ----- SUMMARY -----" << std::endl;
	std::cout << "> Found " << races.load() << " possible data-races" << std::endl;
	std::cout << "> Detector missed " << misses.load() << " possible heap refs" << std::endl;
}

std::string detector::name() {
	return std::string("TSAN");
}

std::string detector::version() {
	return std::string("0.1.0");
}

void detector::acquire(tid_t thread_id, void* mutex) {
	uint64_t addr_32 = lower_half((uint64_t) mutex);
	//printf("[%.5i] Acquire Mutex at: %p\n", thread_id, addr_32);
	__tsan_acquire_use_user_tid((unsigned long)thread_id, (void*)addr_32);
	// TODO: This sometimes fails with error: FATAL: ThreadSanitizer CHECK failed: gotsan.cc:4383 "((s->recursion)) == ((0))" (0xfffffffffffffff2, 0x0)
	//__tsan_mutex_after_lock_use_user_tid((unsigned long)thread_id, (void*)addr_32);
}

void detector::release(tid_t thread_id, void* mutex) {
	uint64_t addr_32 = lower_half((uint64_t)mutex);
	//printf("[%.5i] Release Mutex at: %p\n", thread_id, addr_32);
	__tsan_release_use_user_tid((unsigned long)thread_id, (void*)addr_32);
	// TODO: This sometimes fails with error: FATAL: ThreadSanitizer CHECK failed: gotsan.cc:4383 "((s->recursion)) == ((0))" (0xfffffffffffffff2, 0x0)
	//__tsan_mutex_before_unlock_use_user_tid((unsigned long)thread_id, (void*)addr_32);
}

void detector::read(tid_t thread_id, void* pc, void* addr, unsigned long size) {
	uint64_t addr_32 = lower_half((uint64_t)addr);

	if (!params.heap_only || on_heap((uint64_t)addr_32)) {
		//printf("[%.5i] READ addr tsan: %p\n", thread_id, addr_32);
		//printf("ADDR  raw: %p\n", addr);
		stack_trace[0] = stack_trace[0] = (int*)pc;
		__tsan_read_use_user_tid((unsigned long)thread_id, (void*)addr_32, kSizeLog1, (void*)stack_trace.data(), stack_trace.size(), false, 5, false);
	}
}

void detector::write(tid_t thread_id, void* pc, void* addr, unsigned long size) {
	uint64_t addr_32 = lower_half((uint64_t)addr);

	if (!params.heap_only || on_heap((uint64_t)addr_32)) {
		//printf("[%.5i] WRITE addr tsan: %p\n", thread_id, addr_32);
		stack_trace[0] = (int*)pc;
		__tsan_write_use_user_tid((unsigned long)thread_id, (void*)addr_32, kSizeLog1, (void*)stack_trace.data(), stack_trace.size(), false, 4, false);
	}
}

void detector::alloc(tid_t thread_id, void* pc, void* addr, unsigned long size) {
	uint64_t addr_32 = lower_half((uint64_t)addr);

	mxspin.lock();
	alloc_readable.store(false, std::memory_order_release);

	__tsan_malloc_use_user_tid(thread_id, (unsigned long)pc, addr_32, size);
	allocations.emplace((uint64_t)addr_32, size);

	alloc_readable.store(true, std::memory_order_release);
	mxspin.unlock();
}

void detector::free(tid_t thread_id, void* addr) {
	uint64_t addr_32 = lower_half((uint64_t)addr);
	mxspin.lock();
	alloc_readable.store(false, std::memory_order_release);

	// ocasionally free is called more often than alloc, hence guard
	if (allocations.count(addr_32) == 1) {
		size_t size = allocations[addr_32];
		allocations.erase(addr_32);
		//__tsan_free(addr, size);
		__tsan_reset_range((unsigned int)addr_32, size);
	}

	alloc_readable.store(true, std::memory_order_release);
	mxspin.unlock();
}

void detector::fork(tid_t parent, tid_t child) {
	// TODO
}

void detector::join(tid_t parent, tid_t child) {
	// TODO: Currently not working
	__tsan_end_use_user_tid((unsigned long)child);
}
