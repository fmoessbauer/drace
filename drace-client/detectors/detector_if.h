#pragma once

#include <string>

/**
 * Interface for a drace compatible race detector
 */
namespace detector {
	typedef unsigned int tid_t;
	typedef void*        tls_t;

	/* Takes command line arguments and a callback to demangle the stack at this address.
	* Type of callback is (void*) -> void
	*/
	bool init(int argc, const char **argv, void(*stack_demangler)(void*));
	void finalize();

	// Legacy interface
	void acquire(tid_t thread_id, void* mutex, tls_t tls = nullptr);
	void release(tid_t thread_id, void* mutex, tls_t tls = nullptr);

	void acquire_pre(tid_t thread_id, void* mutex, bool write, tls_t tls = nullptr);
	void acquire_post(tid_t thread_id, void* mutex, bool write, tls_t tls = nullptr);
	void release_pre(tid_t thread_id, void* mutex, bool write, tls_t tls = nullptr);
	void release_post(tid_t thread_id, void* mutex, bool write, tls_t tls = nullptr);

	void read(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);
	void write(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);

	void alloc(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);
	void free(tid_t thread_id, void* addr, tls_t tls);

	void fork(tid_t parent, tid_t child, tls_t tls = nullptr);
	void join(tid_t parent, tid_t child, tls_t tls = nullptr);

	std::string name();
	std::string version();
}
