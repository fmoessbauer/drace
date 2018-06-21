#include "../detector_if.h"

#include <unordered_map>
#include <iostream>
#include "tsan-if.h"

static std::unordered_map<void *, size_t> allocations;

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

bool detector::init() {
	__tsan_init_simple(reportRaceCallBack, (void*)0xABCD);
	return true;
}

void detector::finalize() {
	__tsan_fini();
}

void detector::acquire(tid_t thread_id, void* mutex) {
	__tsan_acquire_use_user_tid(thread_id, (void*)mutex);
}

void detector::release(tid_t thread_id, void* mutex) {
	__tsan_release_use_user_tid(thread_id, (void*)mutex);
}

void detector::read(tid_t thread_id, void* addr, unsigned long size) {
	int* stack_trace[3];
	stack_trace[0] = (int*)0x11;
	stack_trace[1] = (int*)0x33;
	stack_trace[2] = (int*)0x55;

	//TODO: TSAN seems to only support 32 bit addresses!!!
	unsigned long addr_32 = (unsigned long)addr;

	__tsan_read_use_user_tid((unsigned long)thread_id, (void*)addr_32, kSizeLog1, (void*)&stack_trace[0], 3, false, 5, false);
}

void detector::write(tid_t thread_id, void* addr, unsigned long size) {
	int* stack_trace[3];
	stack_trace[0] = (int*)0x11;
	stack_trace[1] = (int*)0x33;
	stack_trace[2] = (int*)0x55;

	//TODO: TSAN seems to only support 32 bit addresses!!!
	unsigned long addr_32 = (unsigned long)addr;

	__tsan_write_use_user_tid((unsigned long)thread_id, (void*)addr_32, kSizeLog1, (void*)&stack_trace[0], 3, false, 4, false);
}

void detector::alloc(tid_t thread_id, void* addr, unsigned long size) {
	uint32_t retoffs = (uint32_t)addr;
	__tsan_malloc_use_user_tid(thread_id, 0, retoffs, size);
	allocations.emplace(addr, size);
}

void detector::free(tid_t thread_id, void* addr) {
	if (allocations.count(addr) == 1) {
		size_t size = allocations[addr];
		allocations.erase(addr);
		//__tsan_free(addr, size);
		__tsan_reset_range((unsigned int)addr, size);
	}
}

void detector::fork(tid_t parent, tid_t child) {
	// TODO
}

void detector::join(tid_t parent, tid_t child) {
	// TODO
}