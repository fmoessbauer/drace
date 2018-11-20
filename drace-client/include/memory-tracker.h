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

		std::atomic<int> flush_active{ false };

		byte         *buf_ptr;
		ptr_int_t     buf_end;
		AlignedBuffer<byte, 64> mem_buf;
		mem_ref_t mem_ref;
		void*       detector_data;
		AlignedStack<void*, 64> stack;

		/// inverse of flush pending, jmpecxz
		std::atomic<ptr_uint_t> no_flush{ false };

		/// inverse of flush pending, jmpecxz
		std::atomic<ptr_uint_t> external_flush{ false };

	private:
		Statistics & _stats;
		thread_id_t _tid;
		bool        _enabled;
		unsigned long _event_cnt;

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

	public:

		MemoryTracker(void* drcontext, Statistics & stats);
		~MemoryTracker();

		void deallocate(void* drcontext);

		void clear_buffer(void);
		void analyze_access();

		inline void process_buffer() {
			if (_enabled) clear_buffer();
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
		 */
		static void update_cache(ThreadState * data);

		static bool pc_in_freq(ThreadState * data, void* bb);

	private:

		/** Read data from external CB and modify instrumentation / detection accordingly */
		void handle_ext_state(ThreadState * data);

		void update_sampling();

		/// needed in instrumentation stage
		friend class Instrumentator;
	};

	extern std::unique_ptr<MemoryTracker> memory_tracker;
}
