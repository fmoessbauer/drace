#include "../detector_if.h"

#include <map>
#include <vector>
#include <atomic>
#include <mutex>

#include <iostream>
#include "tsan-if.h"


/**
 * To avoid false-positives track races only if they are on the heap
 * invert order to get range using lower_bound
 */
// TODO: Make accesses thread safe using memory_order acquire release
// Or use epoches
static std::atomic<bool>		alloc_readable{ true };
static std::atomic<uint64_t>	misses{ 0 };
static std::mutex				alloc_modify_mx;
static std::map<uint64_t, size_t, std::greater<uint64_t>> allocations;
static std::vector<int*> stack_trace;

void reportRaceCallBack(__tsan_race_info* raceInfo, void* callback_parameter) {
	std::cout << "DATA Race " << callback_parameter << ::std::endl;


	for (int i = 0; i != 2; ++i) {
		__tsan_race_info_access* race_info_ac = NULL;

		if (i == 0) {
			race_info_ac = raceInfo->access1;
			std::cout << " Access 1 tid: " << race_info_ac->user_id << " ";

		}
		else {
			race_info_ac = raceInfo->access2;
			std::cout << " Access 2 tid: " << race_info_ac->user_id << " ";
		}
		std::cout << (race_info_ac->write == 1 ? "write" : "read") << " to/from " << race_info_ac->accessed_memory << " with size " << race_info_ac->size << ". Stack(Size " << race_info_ac->stack_trace_size << ")" << "Type: " << race_info_ac->type << " :" << ::std::endl;

		for (int i = 0; i != race_info_ac->stack_trace_size; ++i) {
			std::cout << ((void**)race_info_ac->stack_trace)[i] << std::endl;
		}
	}
}

inline bool on_heap(uint64_t addr) {
	if (alloc_readable.load(std::memory_order_consume)) {
		auto it = allocations.lower_bound(addr);
		uint64_t beg = it->first;
		size_t   sz = it->second;
		return (addr < (beg + sz));
	}
	else {
		// for performance reasons ignore this location
		// TODO: Check impact
		++misses;
		return false;
	}
}

bool detector::init() {
	stack_trace.push_back((int*)0x11);
	stack_trace.push_back((int*)0x33);
	stack_trace.push_back((int*)0x55);

	__tsan_init_simple(reportRaceCallBack, (void*)0xABCD);
	return true;
}

void detector::finalize() {
	// TODO: Currently kills detector
	//__tsan_fini();
	std::cout << "> Detector missed " << misses.load() << " possible heap refs" << std::endl;
}

void detector::acquire(tid_t thread_id, void* mutex) {
	// TODO: Currently kills detector
	//__tsan_acquire_use_user_tid(thread_id, (void*)mutex);
}

void detector::release(tid_t thread_id, void* mutex) {
	// TODO: Currently kills detector
	//__tsan_release_use_user_tid(thread_id, (void*)mutex);
}

void detector::read(tid_t thread_id, void* addr, unsigned long size) {
	if (on_heap((uint64_t)addr)) {
		//TODO: TSAN seems to only support 32 bit addresses!!!
		unsigned long addr_32 = (unsigned long)addr;

		__tsan_read_use_user_tid((unsigned long)thread_id, (void*)addr_32, kSizeLog1, (void*)stack_trace.data(), stack_trace.size(), false, 5, false);
	}
}

void detector::write(tid_t thread_id, void* addr, unsigned long size) {
	if (on_heap((uint64_t)addr)) {
		//TODO: TSAN seems to only support 32 bit addresses!!!
		unsigned long addr_32 = (unsigned long)addr;

		__tsan_write_use_user_tid((unsigned long)thread_id, (void*)addr_32, kSizeLog1, (void*)stack_trace.data(), stack_trace.size(), false, 4, false);
	}
}

void detector::alloc(tid_t thread_id, void* addr, unsigned long size) {
	//std::lock_guard<std::mutex> lg(alloc_modify_mx);
	alloc_readable.store(false, std::memory_order_release);

	uint32_t retoffs = (uint32_t)addr;
	__tsan_malloc_use_user_tid(thread_id, 0, retoffs, size);
	allocations.emplace((uint64_t)addr, size);

	alloc_readable.store(true, std::memory_order_release);
}

void detector::free(tid_t thread_id, void* addr) {
	//std::lock_guard<std::mutex> lg(alloc_modify_mx);
	alloc_readable.store(false, std::memory_order_release);

	// ocasionally free is called more often than alloc, hence guard
	if (allocations.count((uint64_t)addr) == 1) {
		size_t size = allocations[(uint64_t)addr];
		allocations.erase((uint64_t)addr);
		//__tsan_free(addr, size);
		__tsan_reset_range((unsigned int)addr, size);
	}

	alloc_readable.store(true, std::memory_order_release);
}

void detector::fork(tid_t parent, tid_t child) {
	// TODO
}

void detector::join(tid_t parent, tid_t child) {
	// TODO
}