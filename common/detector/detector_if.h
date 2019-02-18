#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <string>
#include <vector>
#include <functional>

/// Interface for a DRace compatible race detector
namespace detector {
	typedef unsigned long tid_t;
	typedef void*         tls_t;

	static constexpr int max_stack_size = 16;

	/** A single memory access */
	struct AccessEntry {
		unsigned thread_id;
		bool     write;
		uint64_t accessed_memory;
		size_t   access_size;
		int      access_type;
		uint64_t heap_block_begin;
		size_t   heap_block_size;
		bool     onheap{ false };
		size_t   stack_size{ 0 };
		uint64_t stack_trace[max_stack_size];
	};

	/** A Data-Race is a tuple of two Accesses */
	using Race = std::pair<AccessEntry, AccessEntry>;

	using Callback = void(*)(const detector::Race*);

	/**
	* Takes command line arguments and a callback to process a data-race.
	* Type of callback is (const detector::Race*) -> void
	*/
	bool init(int argc, const char **argv, Callback rc_clb);

	/**
	 * Finalizes the detector.
	 * After a finalize, a later init must be possible
	 */
	void finalize();

	/** Acquire a mutex */
	void acquire(
		/// ptr to thread-local storage of calling thread
		tls_t tls,
		/// ptr to mutex location
		void* mutex,
		/// number of recursive locks (1 for non-recursive mutex) 
		int   recursive,
		/// true, for RW-mutexes in read-mode false
		bool  write
	);

	/** Release a mutex */
	void release(
		/// ptr to thread-local storage of calling thread
		tls_t tls,
		/// ptr to mutex location
		void* mutex,
		/// true, for RW-mutexes in read-mode false
		bool  write
	);

	/** Draw a happens-before edge between thread and identifier (optional) */
	void happens_before(tid_t thread_id, void* identifier);
	/** Draw a happens-after edge between thread and identifier (optional) */
	void happens_after(tid_t thread_id, void* identifier);

	/** Log a read access */
	void read(
		/// ptr to thread-local storage of calling thread
		tls_t    tls,
		/// array of stack pointers, bottom is current ip
		void*    callstack,
		/// size of callstack
		unsigned stacksize,
		/// memory location
		void*    addr,
		/// access size log2 (bytes)
		size_t   size
	);

	/** Log a write access */
	void write(
		/// ptr to thread-local storage of calling thread
		tls_t    tls,
		/// array of stack pointers, bottom is current ip
		void*    callstack,
		/// size of callstack
		unsigned stacksize,
		/// memory location
		void*    addr,
		/// access size log2 (bytes)
		size_t   size
	);

	/** Log a memory allocation */
	void allocate(
		/// ptr to thread-local storage of calling thread
		tls_t  tls,
		/// current instruction pointer
		void*  pc,
		/// begin of allocated memory block
		void*  addr,
		/// size of memory block
		size_t size
	);

	/** Log a memory deallocation*/
	void deallocate(
		/// ptr to thread-local storage of calling thread
		tls_t tls,
		/// begin of memory block
		void* addr
	);

	/** Log a thread-creation event */
	void fork(
		/// id of parent thread
		tid_t parent,
		/// id of child thread
		tid_t child,
		/// out parameter for tls data
		tls_t * tls
	);

	/** Log a thread join event */
	void join(
		/// id of parent thread
		tid_t parent,
		/// id of child thread
		tid_t child,
		/// tls of child thread
		tls_t tls
	);

	/** Log a thread detach event */
	void detach(tid_t thread_id, tls_t tls);
	/** Log a thread exit event (detached thread) */
	void finish(tid_t thread_id, tls_t tls);

	/** Return name of detector */
	std::string name();
	/** Return version of detector */
	std::string version();
}
