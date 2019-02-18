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

#include "globals.h"
#include "aligned-stack.h"
#include "memory-tracker.h"
#include <iterator>
#include <dr_api.h>
#include <drmgr.h>

namespace drace {
	/**
	* This class implements a shadow stack, used for
	* getting stack traces for race detection
	* Whenever a call has been detected, the memory-refs buffer is
	* flushed as all memory-refs happend in the current function.
	* This avoids storing a stack-trace per memory-entry.
	*/
	class ShadowStack {
	public:
		/// four cache-lines - one element which contains current mem pc
		static constexpr int max_size = 31;
		using stack_t = decltype(per_thread_t::stack);

	private:
		/** Push a pc on the stack. If the pc is already
		* on the stack, skip. This avoids ever growing stacks
		* if the returns are not detected properly
		*/
		static inline void push(void *addr, stack_t* stack)
		{
			auto size = stack->entries;
			if (size >= max_size) return;

			// TODO: if not all modules get the shadow-stack
			// instrumentation, we have to clear here as we
			// miss return statements
#if 0
		// sometimes we miss return statements, hence check
		// if pc is new
			for (unsigned i = 0; i < size; ++i)
				if (stack->data[i] == addr) return;
#endif

			stack->data[stack->entries++] = addr;
		}

		static inline void *pop(stack_t* stack)
		{
			DR_ASSERT(stack->entries > 0);
			stack->entries--;
			return stack->data[stack->entries];
		}

		/** Call Instrumentation */
		static void on_call(void *call_ins, void *target_addr)
		{
			void * drcontext = dr_get_current_drcontext();
			DR_ASSERT(!dr_using_app_state(drcontext));

			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(drcontext, tls_idx);
			if (!params.fastmode) {
				while (data->external_flush.load(std::memory_order_relaxed)) {
					// wait
				}
			}
			// TODO: possibibly racy in non-fast-mode
			MemoryTracker::analyze_access(data);

			// Sampling: Possibly disable detector during this function
			memory_tracker->switch_sampling(data);
			
			// if lossy_flush, disable detector instead of changeing the instructions
			if (params.lossy && !params.lossy_flush && MemoryTracker::pc_in_freq(data, call_ins)) {
				data->enabled = false;
			}

			push(call_ins, &(data->stack));
		}

		/** Return Instrumentation */
		static void on_ret(void *ret_ins, void *target_addr)
		{
			per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(dr_get_current_drcontext(), tls_idx);
			stack_t * stack = &(data->stack);

			if (!params.fastmode) {
				while (data->external_flush.load(std::memory_order_relaxed)) {
					// wait
				}
			}
			MemoryTracker::analyze_access(data);

			if (stack->entries == 0) return;

			ptrdiff_t diff;
			while ((diff = (byte*)target_addr - (byte*)pop(stack)), !(0 <= diff && diff <= 8))
			{
				if (stack->entries == 0) return;
				// skipping a frame
			}
		}

	public:
		static void instrument(void *drcontext, void *tag, instrlist_t *bb,
			instr_t *instr, bool for_trace,
			bool translating, void *user_data)
		{
			if (instr == instrlist_last(bb)) {
				if (instr_is_call_direct(instr))
					dr_insert_call_instrumentation(drcontext, bb, instr, on_call);
				else if (instr_is_call_indirect(instr))
					dr_insert_mbr_instrumentation(drcontext, bb, instr, on_call, SPILL_SLOT_1);
				else if (instr_is_return(instr))
					dr_insert_mbr_instrumentation(drcontext, bb, instr, on_ret, SPILL_SLOT_1);
			}
		}
	};
} // namespace drace