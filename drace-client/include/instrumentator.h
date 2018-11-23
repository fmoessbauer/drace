#pragma once

#include "globals.h"
#include "memory-tracker.h"
#include "module/Cache.h"

namespace drace {

	class ThreadState;

	class Instrumentator {
	private:
		module::Cache mc;

		size_t page_size;

		/** Code Caches */
		app_pc cc_flush;

		/** XCX registers */
		drvector_t allowed_xcx;

		/** Number of instrumented calls, used for sampling
		 * \Warning This number is racily incremented by design.
		 *          Hence, use the largest native type to avoid partial loads
		 */
		unsigned long long instrum_count{ 0 };

	public:
		Instrumentator();
		~Instrumentator();

		void event_thread_init(void *drcontext);
		void event_thread_exit(void *drcontext);

		inline void flush_region(void* drcontext, uint64_t pc) {
			// Flush this area from the code cache
			dr_delay_flush_region((app_pc)(pc << MemoryTracker::HIST_PC_RES), 1 << MemoryTracker::HIST_PC_RES, 0, NULL);
			LOG_NOTICE(dr_get_thread_id(drcontext), "Flushed %p", pc << MemoryTracker::HIST_PC_RES);
		}

	private:
		void code_cache_init(void);
		void code_cache_exit(void);


		// Instrumentation
		/** Inserts a jump to clean call if a flush is pending */
		void insert_jmp_on_flush(void *drcontext, instrlist_t *ilist, instr_t *where,
			reg_id_t regxcx, reg_id_t regtls, instr_t *call_flush);

		/** Instrument all memory accessing instructions */
		void instrument_mem_full(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write);
		/** Instrument all memory accessing instructions (fast-mode)*/
		void instrument_mem_fast(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write);

		/**
		* instrument_mem is called whenever a memory reference is identified.
		* It inserts code before the memory reference to to fill the memory buffer
		* and jump to our own code cache to call the clean_call when the buffer is full.
		*/
		inline void instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref, bool write) {
			if (params.fastmode) {
				instrument_mem_fast(drcontext, ilist, where, ref, write);
			}
			else {
				//instrument_mem_full(drcontext, ilist, where, ref, write);
			}
		}

		/** Update the code cache and remove items where the instrumentation should change.
		 * We only consider traces, as other parts are not performance critical
         */
		static void update_cache(ThreadState * data);

		/** We transform string loops into regular loops so we can more easily
		* monitor every memory reference they make.
		*/
		dr_emit_flags_t event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);

		dr_emit_flags_t event_app_analysis(void * drcontext, void * tag, instrlist_t * bb, bool for_trace, bool translating, OUT void ** user_data);
		/** Instrument application instructions */
		dr_emit_flags_t event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
			instr_t *instr, bool for_trace,
			bool translating, void *user_data);

		static inline dr_emit_flags_t instr_event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
		{
			return instrumentator->event_bb_app2app(drcontext, tag, bb, for_trace, translating);
		}

		static inline dr_emit_flags_t instr_event_app_analysis(void * drcontext, void * tag, instrlist_t * bb, bool for_trace, bool translating, OUT void ** user_data)
		{
			return instrumentator->event_app_analysis(drcontext, tag, bb, for_trace, translating, user_data);
		}

		static inline dr_emit_flags_t instr_event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
			instr_t *instr, bool for_trace,
			bool translating, void *user_data)
		{
			return instrumentator->event_app_instruction(drcontext, tag, bb, instr, for_trace, translating, user_data);
		}
	};

} // namespace drace
