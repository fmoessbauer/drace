#include "../detector_if.h"
#include <iostream>

bool detector::init(int argc, const char **argv, void(*stack_demangler)(void*)) {
	return true;
}

void detector::finalize() { }

void detector::acquire(tid_t thread_id, void* mutex) { }

void detector::release(tid_t thread_id, void* mutex) { }

void detector::read(tid_t thread_id, void* pc, void* addr, unsigned long size) { }

void detector::write(tid_t thread_id, void* pc, void* addr, unsigned long size) { }

void detector::alloc(tid_t thread_id, void* addr, unsigned long size) { }

void detector::free(tid_t thread_id, void* addr) { }

void detector::fork(tid_t parent, tid_t child) { }

void detector::join(tid_t parent, tid_t child) { }