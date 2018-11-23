#pragma once

#include <memory>
#include <unordered_map>

#include "memory-tracker.h"
#include "statistics.h"

#include <dr_api.h>

namespace drace {

	/** Per Thread data (thread-private) */
	class alignas(64) ThreadState {
	public:
		ThreadState(void* drcontext) :
			stats(dr_get_thread_id(drcontext)),
			mtrack(drcontext, stats) { }

		~ThreadState() = default;

		ThreadState(const ThreadState &) = delete;
		ThreadState(ThreadState &&) = default;

		ThreadState & operator=(const ThreadState &) = delete;
		ThreadState & operator=(ThreadState &&) = default;

		inline void deallocate(void* drcontext) {
			mtrack.deallocate(drcontext);
		}

	public:
		/// Memory Tracker. This has to be aligned to a cache-line
		MemoryTracker mtrack;

		thread_id_t   tid;

		/** book-keeping of active mutexes
		 * All even indices are mutex addresses
		 * while uneven indices denote the number of
		 * references at the location in index-1.
		 * This is tuned for maximum cache-locality */
		std::unordered_map<uint64_t, unsigned> mutex_book;
		/// Statistics
		Statistics   stats;
		/// allocation begin of tls (without alignment)
		void * alloc_begin;
	};
}