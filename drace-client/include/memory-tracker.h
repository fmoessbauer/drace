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

#include "Module.h"
#include "statistics.h"

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include <atomic>
#include <memory>
#include <random>

namespace drace {
	/**
	* \brief Covers application memory tracing.
    *
	* Responsible for adding all instrumentation code (except function wrapping).
	* Callbacks from the code-cache are handled in this class.
	* Memory-Reference analysis is done here.
	*/
	class MemoryTracker {
	public:

		/// Single memory reference
		struct mem_ref_t {
			void    *addr;
            app_pc   pc;
			uint32_t size;
            bool     write;
		};

		/// Maximum number of references between clean calls
		static constexpr int MAX_NUM_MEM_REFS = 64;
		static constexpr int MEM_BUF_SIZE = sizeof(mem_ref_t) * MAX_NUM_MEM_REFS;

		/// aggregate frequent pc's on this granularity (2^n bytes)
		static constexpr unsigned HIST_PC_RES = 10;

		/// update code-cache after this number of flushes (must be power of two)
		static constexpr unsigned CC_UPDATE_PERIOD = 1024 * 64;

		std::atomic<int> flush_active{ false };

	private:
		size_t page_size;

		/// Code Caches
		app_pc cc_flush;

		/**
         * \brief Number of instrumented calls, used for sampling
         *
		 * \Warning This number is racily incremented by design.
		 *          Hence, use the largest native type to avoid partial loads
		 */
		unsigned long long instrum_count{ 0 };

		/* XCX registers */
		drvector_t allowed_xcx;

		module::Cache mc;

		/// fast random numbers for sampling
		std::mt19937 _prng;

		/// maximum length of period
		unsigned _min_period = 1;
		/// minimum length of period
		unsigned _max_period = 1;
		/// current pos in period
		int _sample_pos = 0;

		static const std::mt19937::result_type _max_value = decltype(_prng)::max();

	public:

		MemoryTracker();
		~MemoryTracker();

		static void process_buffer(void);
		static void clear_buffer(void);
		static void analyze_access(per_thread_t * data);
		static void flush_all_threads(per_thread_t * data, bool self = true, bool flush_external = false);

		// Events
		void event_thread_init(void *drcontext);

		void event_thread_exit(void *drcontext);

		/** We transform string loops into regular loops so we can more easily
		* monitor every memory reference they make.
		*/
		dr_emit_flags_t event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);

		dr_emit_flags_t event_app_analysis(void * drcontext, void * tag, instrlist_t * bb, bool for_trace, bool translating, OUT void ** user_data);
		/** Instrument application instructions */
		dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
			instr_t *instr, bool for_trace,
			bool translating, void *user_data);

		/// Returns true if this reference should be sampled
		inline bool sample_ref(per_thread_t * data) {
			--data->sampling_pos;
			if (params.sampling_rate == 1)
				return true;
			if (data->sampling_pos == 0) {
				data->sampling_pos = std::uniform_int_distribution<unsigned>{ memory_tracker->_min_period, memory_tracker->_max_period }(memory_tracker->_prng);
				return true;
			}
			return false;
		}

		/// Sets the detector state based on the sampling condition
		inline void switch_sampling(per_thread_t * data) {
			if (!sample_ref(data)) {
				data->enabled = false;
				data->event_cnt |= ((uint64_t)1 << 63);
			}
			else {
				data->event_cnt &= ~((uint64_t)1 << 63);
				if (!data->event_cnt)
					data->enabled = true;
			}
		}

		/// enable the detector (does not affect sampling)
		static inline void enable(per_thread_t * data) {
			// access the lower part of the 64bit uint
			*((uint32_t*)(&(data->enabled))) = true;
		}

		/// disable the detector (does not affect sampling)
		static inline void disable(per_thread_t * data) {
			// access the lower part of the 64bit uint
			*((uint32_t*)(&(data->enabled))) = false;
		}

		/// enable the detector during this scope
		static inline void enable_scope(per_thread_t * data) {
			if (--data->event_cnt <= 0) {
				enable(data);
			}
		}

		/// disable the detector during this scope
		static inline void disable_scope(per_thread_t * data) {
			disable(data);
			data->event_cnt++;
		}

		/**
         * \brief Update the code cache and remove items where the instrumentation should change.
         *
		 * We only consider traces, as other parts are not performance critical
		 */
		static void update_cache(per_thread_t * data);

		static bool pc_in_freq(per_thread_t * data, void* bb);

	private:

		void code_cache_init(void);
		void code_cache_exit(void);

		// Instrumentation
		/// Inserts a jump to clean call if a flush is pending
		void insert_jmp_on_flush(void *drcontext, instrlist_t *ilist, instr_t *where,
			reg_id_t regxcx, reg_id_t regtls, instr_t *call_flush);

		/// Instrument all memory accessing instructions
		void instrument_mem_full(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write);

		/// Instrument all memory accessing instructions (fast-mode)
		void instrument_mem_fast(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write);

		/**
		* \brief instrument_mem is called whenever a memory reference is identified.
        *
		* It inserts code before the memory reference to to fill the memory buffer
		* and jump to our own code cache to call the clean_call when the buffer is full.
		*/
		inline void instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write) {
			instrument_mem_fast(drcontext, ilist, where, ref, write);
		}

		/// Read data from external CB and modify instrumentation / detection accordingly
		void handle_ext_state(per_thread_t * data);

		void update_sampling();
	};

	extern std::unique_ptr<MemoryTracker> memory_tracker;

	static inline dr_emit_flags_t instr_event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
	{
		return memory_tracker->event_bb_app2app(drcontext, tag, bb, for_trace, translating);
	}

	static inline dr_emit_flags_t instr_event_app_analysis(void * drcontext, void * tag, instrlist_t * bb, bool for_trace, bool translating, OUT void ** user_data)
	{
		return memory_tracker->event_app_analysis(drcontext, tag, bb, for_trace, translating, user_data);
	}

	static inline dr_emit_flags_t instr_event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
		instr_t *instr, bool for_trace,
		bool translating, void *user_data)
	{
		return memory_tracker->event_app_instruction(drcontext, tag, bb, instr, for_trace, translating, user_data);
	}

}
