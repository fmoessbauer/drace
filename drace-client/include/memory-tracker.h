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
	class ShadowStack;

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

	private:
		/** Control Block used in inline instrumentation,
		* should fit into one cache line.
		* Layout:
		*@code
		*|-------|---63 bit---|
		*|enabled|sampling-pos|
		*@endcode
		*/
		uint64_t	  _ctrlblock = 1;
		/// last calculated sampling period (must not be zero)
		uint64_t	  _sampling_period = 1;

		/// pointer to the next memory slot
		mem_ref_t   * _buf_ptr;
		/// negative value of buffer+size
		ptr_int_t     _buf_end;
		/// buffer for memory references
		mem_ref_t     _mem_buf[MAX_NUM_MEM_REFS];

		/// begin of this threads stack range
		ULONG_PTR _appstack_beg{ 0x0 };
		/// end of this threads stack range
		ULONG_PTR _appstack_end{ 0x0 };
		/**
		 * shadow stack. We keep it in a different cache line (it allocates),
		 * as it is not needed during inline asm memory tracking
		 */
		AlignedStack<void*, 64> stack;
	public:
		/// dr's id of current thread
		thread_id_t tid;
		/// true if an orchestrated flush is currently running (applies only to non fast-mode)
		std::atomic<int> flush_active{ false };

		/**
		 * as the detector cannot allocate TLS,
		 * use this ptr for per-thread data in detector */
		void*       detector_data{nullptr};

		/// inverse of flush pending, jmpecxz
		std::atomic<ptr_uint_t> no_flush{ false };

		/// inverse of flush pending, jmpecxz
		std::atomic<ptr_uint_t> external_flush{ false };

		/// Used for event syncronisation procedure
		using tls_map_t = std::vector<std::pair<thread_id_t, MemoryTracker&>>;
		tls_map_t     th_towait;

	private:
		/// global statistics collector
		Statistics & _stats;

		/** bool external change detected
		* this flag is used to trigger the enable or disable
		* logic on this thread */
		bool        _enable_external{ true };
		/**
		* Tracks number of nested excluded regions
		*/
		unsigned long _event_cnt{0};

		/// fast random numbers to calculate next sampling period length
		std::mt19937 _prng;

		/// maximum length of period
		unsigned _min_period = 1;
		/// minimum length of period
		unsigned _max_period = 1;

		static constexpr std::mt19937::result_type _max_value = decltype(_prng)::max();

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

		/**
		 * deallocate this object using the provided drcontext.
		 * \note: we cannot rely on RAII here, as memory allocated using
		 *        dr has to be deallocated with a specific context.
		 *        In the thread_exit event, this context might differ
		 *        from \cdr_get_current_drcontext();
		 */
		void deallocate(void* drcontext);

		/**
		* drop all memory references in the current buffer
		*/
		void clear_buffer(void);
		/**
		* analyze memory buffer
		*/
		void analyze_access();

		/**
		* process the current memory buffer (analyze / drop) based on the
		* detector state
		*/
		inline void process_buffer() {
			if (!is_enabled()) clear_buffer();
			else analyze_access();
		}

		/**
		* enter a scope where the detector is disabled.
		* Nested scopes are supported
		*/
		inline void disable_scope() {
			disable();
			++_event_cnt;
		}
		/**
		* leave a possibly nested disabled scope.
		*/
		inline void enable_scope() {
			if (_event_cnt <= 1) {
				//memory_tracker->clear_buffer();
				enable();
				_event_cnt = 0;
				return;
			}
			--_event_cnt;
		}

		/// enable the detector
		inline void enable() {
			_ctrlblock &= ~((uint64_t)1 << 63);
			DR_ASSERT(is_enabled());
		}

		/// disable the detector
		inline void disable() {
			_ctrlblock |= (uint64_t)1 << 63;
			DR_ASSERT(!is_enabled());
		}

		/// true if the detector is currently enabled
		constexpr bool is_enabled() {
			return !(_ctrlblock & ((uint64_t)1 << 63));
		}

		/// returns the current position in the sampling period. 
		constexpr uint32_t get_sample_pos() {
			// cast away flags field
			return static_cast<uint32_t>(_ctrlblock);
		}

		/** Returns true if this reference should be sampled.
		*   \note only available in non fast-mode
		*/
		inline bool sample_ref() {
			if (_sampling_period == 1)
				return true;
			// we decrement the lower half only (little endian)
			uint32_t* pos = ((uint32_t*)&_ctrlblock);
			DR_ASSERT(*pos != 0);
			--(*pos);
			if (*pos == 0) {
				*pos = std::uniform_int_distribution<unsigned>{ _min_period, _max_period }(_prng);
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

		/**
		 * static version of process_buffer to be called from code cache
		 */
		static void process_buffer_static();

		/** Request a flush of all non-disabled threads.
		*  \Warning: This function read-locks the TLS mutex
		*/
		static void flush_all_threads(MemoryTracker & mtr, bool self = true, bool flush_external = false);

		/// true if the passed in basic block is frequent
		static bool pc_in_freq(ThreadState * data, void* bb);

	private:

		/** Read data from external CB and modify instrumentation / detection accordingly */
		void handle_ext_state();
		/** recalculate sampling boundary based on external data */
		void update_sampling();

		/// needed in instrumentation stage
		friend class Instrumentator;
		friend class ShadowStack;
	};
}
