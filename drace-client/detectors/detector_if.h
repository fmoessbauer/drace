#pragma once

#include <string>
#include <vector>

/**
 * Interface for a drace compatible race detector
 */
namespace detector {
	typedef unsigned int tid_t;
	typedef void*        tls_t;

	/* A single memory access */
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

	/* A Data-Race is a tuple of two Accesses*/
	using Race = std::pair<AccessEntry, AccessEntry>;

	/* Takes command line arguments and a callback to process a data-race.
	* Type of callback is (const detector::Race*) -> void
	*/
	bool init(int argc, const char **argv, void(*rc_clb)(const detector::Race*));

	/* Finalizes the detector.
	 * After a finalize, a later init must be possible
	 */
	void finalize();

	/* Acquire a mutex */
	void acquire(
		tid_t thread_id,
		void* mutex,
		int recursive = 1,
		bool write = true,
		bool try_lock = false,
		tls_t tls = nullptr);

	/* Release a mutex */
	void release(
		tid_t thread_id,
		void* mutex,
		bool write = true,
		tls_t tls = nullptr);

	/* Draw a happens-before edge between thread and identifier (optional) */
	void happens_before(tid_t thread_id, void* identifier);
	/* Draw a happens-after edge between thread and identifier (optional) */
	void happens_after(tid_t thread_id, void* identifier);

	/* Log a read access */
	void read(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);
	/* Log a write access */
	void write(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);

	/* Log a memory allocation */
	void allocate(tid_t thread_id, void* pc, void* addr, unsigned long size, tls_t tls = nullptr);
	/* Log a memory deallocation*/
	void deallocate(tid_t thread_id, void* addr, tls_t tls);

	/* Log a thread-creation event */
	void fork(tid_t parent, tid_t child, tls_t tls = nullptr);
	/* Log a thread join event */
	void join(tid_t parent, tid_t child, tls_t tls = nullptr);
	/* Log a thread detach event */
	void detach(tid_t thread_id, tls_t tls);
	/* Log a thread exit event (detached thread) */
	void finish(tid_t thread_id, tls_t tls);

	/* Return name of detector */
	std::string name();
	/* Return version of detector */
	std::string version();
}
