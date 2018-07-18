#pragma once

#include <string>
#include <vector>

/**
 * Interface for a drace compatible race detector
 */
namespace detector {
	typedef unsigned int tid_t;
	typedef void*        tls_t;

	struct AccessEntry {
		unsigned thread_id;
		bool     write;
		void    *accessed_memory;
		size_t   access_size;
		int      access_type;
		std::vector<uint64_t> stack_trace;
		void    *heap_block_begin;
		size_t   heap_block_size;
		bool     onheap;
	};

	using Race = std::pair<AccessEntry, AccessEntry>;

	/* Takes command line arguments and a callback to demangle the stack at this address.
	* Type of callback is (void*) -> void
	*/
	bool init(int argc, const char **argv, void * add_race_clb);
	void finalize();

	void acquire(
		tid_t thread_id,
		void* mutex,
		int recursive = 1,
		bool write = true,
		bool try_lock = false,
		tls_t tls = nullptr);

	void release(
		tid_t thread_id,
		void* mutex,
		bool write = true,
		tls_t tls = nullptr);

	void read(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);
	void write(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);

	void alloc(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);
	void free(tid_t thread_id, void* addr, tls_t tls);

	void fork(tid_t parent, tid_t child, tls_t tls = nullptr);
	void join(tid_t parent, tid_t child, tls_t tls = nullptr);
	void detach(tid_t thread_id, tls_t tls);
	void finish(tid_t thread_id, tls_t tls);

	std::string name();
	std::string version();
}
