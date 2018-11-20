#pragma once

#include <memory>
#include <unordered_map>

#include "memory-tracker.h"
#include "statistics.h"

#include <dr_api.h>

namespace drace {

	/** Per Thread data (thread-private) */
	class ThreadState {
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
		using tls_map_t = std::vector<std::pair<thread_id_t, ThreadState*>>;

		thread_id_t   tid;
		/// bool external change detected
		/// this flag is used to trigger the enable or disable
		/// logic on this thread
		bool enable_external{ true };

		/** book-keeping of active mutexes
		 * All even indices are mutex addresses
		 * while uneven indices denote the number of
		 * references at the location in index-1.
		 * This is tuned for maximum cache-locality */
		std::unordered_map<uint64_t, unsigned> mutex_book;
		/// Used for event syncronisation procedure
		tls_map_t     th_towait;
		/// Statistics
		Statistics   stats;
		/// Memory Tracker
		MemoryTracker mtrack;
		/// local sampling state
		int sampling_pos = 0;
		/**
		 * as the detector cannot allocate TLS,
		 * use this ptr for per-thread data in detector */
		void         *detector_data{ nullptr };
	};
}