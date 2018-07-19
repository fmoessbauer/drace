#include "../detector_if.h"
#include <string>

bool detector::init(int argc, const char **argv, void(*rc_clb)(detector::Race*)) {
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

std::string detector::name() {
	return std::string("Dummy");
}

std::string detector::version() {
	return std::string("1.0.0");
}
