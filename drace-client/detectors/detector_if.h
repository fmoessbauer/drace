#pragma once

#include <string>

/**
 * Interface for a drace compatible race detector
 */
namespace detector {
	typedef unsigned int tid_t;

	/* Takes command line arguments and a callback to demangle the stack at this address.
	* Type of callback is (void*) -> void
	*/
	bool init(int argc, const char **argv, void(*stack_demangler)(void*));
	void finalize();

	void acquire(tid_t thread_id, void* mutex);
	void release(tid_t thread_id, void* mutex);

	void read(tid_t thread_id, void* pc, void* addr, unsigned long size);
	void write(tid_t thread_id, void* pc, void* addr, unsigned long size);

	void alloc(tid_t thread_id, void* addr, unsigned long size);
	void free(tid_t thread_id, void* addr);

	void fork(tid_t parent, tid_t child);
	void join(tid_t parent, tid_t child);

	std::string name();
	std::string version();
}
