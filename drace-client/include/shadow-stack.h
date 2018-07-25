#pragma once

#include "globals.h"
#include "memory-tracker.h"
#include <iterator>
#include <dr_api.h>
#include <drmgr.h>
#include <drvector.h>

class ShadowStack {
	static constexpr int max_size = 20;
	static void push(void *addr, drvector_t* stack)
	{
		auto size = stack->entries;
		if (size > max_size) return;
		// sometimes we miss return statements, hence check
		// if pc is new
		for (int i = 0; i < size; ++i)
			if (stack->array[i] == addr) return;

		drvector_append(stack, addr);
	}

	static void *pop(drvector_t* stack)
	{
		DR_ASSERT(stack->entries > 0);
		stack->entries--;
		return stack->array[stack->entries];
	}

	static void on_call(void *call_ins, void *target_addr)
	{
		per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(dr_get_current_drcontext(), tls_idx);
		push(call_ins, &(data->stack));
		MemoryTracker::analyze_access(data);
	}

	static void on_ret(void *ret_ins, void *target_addr)
	{
		per_thread_t * data = (per_thread_t*)drmgr_get_tls_field(dr_get_current_drcontext(), tls_idx);
		drvector_t * stack = &(data->stack);

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
