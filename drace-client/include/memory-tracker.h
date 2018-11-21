#pragma once
#include "aligned-buffer.h"

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <atomic>
#include <memory>
#include <random>

// windows defines max()
#undef max

namespace drace {
	class ThreadState;
	class Statistics;

	/**
	* Covers application memory tracing.
	* Responsible for adding all instrumentation code (except function wrapping).
	* Callbacks from the code-cache are handled in this class.
	* Memory-Reference analysis is done here.
	*/
	class MemoryTracker {
	public:
		/** Single memory reference */
		struct mem_ref_t {
			bool  write;
			void *addr;
			size_t size;
			app_pc pc;
		};

		/** Maximum number of references between clean calls */
		static constexpr int MAX_NUM_MEM_REFS = 128;
		static constexpr int MEM_BUF_SIZE = sizeof(mem_ref_t) * MAX_NUM_MEM_REFS;

		/** aggregate frequent pc's on this granularity (2^n bytes)*/
		static constexpr unsigned HIST_PC_RES = 10;
		/** update code-cache after this number of flushes (must be power of two) */
		static constexpr unsigned CC_UPDATE_PERIOD = 1024 * 64;

		thread_id_t tid;

		byte         *buf_ptr;
		ptr_int_t     buf_end;
		AlignedBuffer<byte, 64> mem_buf;

		std::atomic<int> flush_active{ false };

		/**
		 * as the detector cannot allocate TLS,
		 * use this ptr for per-thread data in detector */
		void*       detector_data{nullptr};
		AlignedStack<void*, 64> stack;

		/// inverse of flush pending, jmpecxz
		std::atomic<ptr_uint_t> no_flush{ false };

		/// inverse of flush pending, jmpecxz
		std::atomic<ptr_uint_t> external_flush{ false };

		/// Used for event syncronisation procedure
		using tls_map_t = std::vector<std::pair<thread_id_t, MemoryTracker&>>;
		tls_map_t     th_towait;

	private:
		Statistics & _stats;
		bool        _enabled{true};

		/** bool external change detected
		* this flag is used to trigger the enable or disable
		* logic on this thread */
		bool        _enable_external{ true };
		unsigned long _event_cnt{0};

		/// begin of this threads stack range
		ULONG_PTR _appstack_beg{ 0x0 };
		/// end of this threads stack range
		ULONG_PTR _appstack_end{ 0x0 };

		// fast random numbers for sampling
		std::mt19937 _prng;

		unsigned _sampling_period = 1;
		/// maximum length of period
		unsigned _min_period = 1;
		/// minimum length of period
		unsigned _max_period = 1;
		/// current pos in period
		int _sample_pos = 0;

		static const std::mt19937::result_type _max_value = decltype(_prng)::max();

		/// used to track deallocation / deconstruction. After deallocation, live==false
		bool live{ true };

	public:

		/** Setup memory tracking of this thread.
		* \Note We do not need the prng here, as it crashes dr in 
		*       thread-init-event. Instead we use the first call
		*       from the code cache
		*/
		MemoryTracker(void* drcontext, Statistics & stats);
		/// Finalize memory tracking of this thread
		~MemoryTracker();

		void deallocate(void* drcontext);

		void clear_buffer(void);
		void analyze_access();

		inline void process_buffer() {
			if (!_enabled) clear_buffer();
			else analyze_access();
		}

		inline void disable_scope() {
			_enabled = false;
			++_event_cnt;
		}

		inline void enable_scope() {
			if (_event_cnt <= 1) {
				//memory_tracker->clear_buffer();
				_enabled = true;
				// recover from missed events
				if (_event_cnt < 0)
					_event_cnt = 1;
			}
			_event_cnt--;
		}

		/**
		* static version of process_buffer to be called from code cache
		*/
		static void process_buffer_static();

		/* Request a flush of all non-disabled threads.
		*  \Warning: This function read-locks the TLS mutex
		*/
		static void flush_all_threads(MemoryTracker & mtr, bool self = true, bool flush_external = false);

		// Events
		void event_thread_init(void *drcontext);

		void event_thread_exit(void *drcontext);

		/** Returns true if this reference should be sampled */
		inline bool sample_ref() {
			--_sample_pos;
			if (_sampling_period == 1)
				return true;
			if (_sample_pos < 0) {
				_sample_pos = std::uniform_int_distribution<unsigned>{ _min_period, _max_period }(_prng);
				return true;
			}
			return false;
		}

		/** Update the code cache and remove items where the instrumentation should change.
		 * We only consider traces, as other parts are not performance critical
		 * \TODO: Currently this only works well with thread-local code caches
		 *        We either should use a global state (+sync), or drop this approach
		 */
		void update_cache();

		static bool pc_in_freq(ThreadState * data, void* bb);

	private:

		/** Read data from external CB and modify instrumentation / detection accordingly */
		void handle_ext_state();

		void update_sampling();

		/// needed in instrumentation stage
		friend class Instrumentator;
	};
}
