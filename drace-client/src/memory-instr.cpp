#pragma once

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drutil.h>
#include <dr_tools.h>

#include "memory-instr.h"

bool memory_inst::register_events() {
	return (
		drmgr_register_thread_init_event(memory_inst::event_thread_init) &&
		drmgr_register_thread_exit_event(memory_inst::event_thread_exit) &&
		drmgr_register_bb_app2app_event(event_bb_app2app, NULL)          &&
		drmgr_register_bb_instrumentation_event(NULL, event_app_instruction, NULL)
		);
}

/* allocate thread local storage
currently not necessary, as we call tsan directly */
void memory_inst::allocate_tls() {
	//tls_idx = drmgr_register_tls_field();
	//DR_ASSERT(tls_idx != -1);
	///* The TLS field provided by DR cannot be directly accessed from the code cache.
	//* For better performance, we allocate raw TLS so that we can directly
	//* access and update it with a single instruction.
	//*/
	//if (!dr_raw_tls_calloc(&tls_seg, &tls_offs, MEMTRACE_TLS_COUNT, 0))
	//	DR_ASSERT(false);
}

void memory_inst::finalize() {
	if (!drmgr_unregister_thread_init_event(memory_inst::event_thread_init) ||
		!drmgr_unregister_thread_exit_event(memory_inst::event_thread_exit) ||
		!drmgr_unregister_bb_app2app_event(memory_inst::event_bb_app2app) ||
		!drmgr_unregister_bb_insertion_event(memory_inst::event_app_instruction) ||
		drreg_exit() != DRREG_SUCCESS)
		DR_ASSERT(false);
}

void memory_inst::at_access_mem(opnd_t opcode, opnd_t address) {
	++num_calls;
}

// Events
void memory_inst::event_thread_init(void *drcontext)
{
}

void memory_inst::event_thread_exit(void *drcontext)
{
	dr_printf("< Num calls %i\n", num_calls.load());
}


/* We transform string loops into regular loops so we can more easily
* monitor every memory reference they make.
*/
dr_emit_flags_t memory_inst::event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
{
	if (!drutil_expand_rep_string(drcontext, bb)) {
		DR_ASSERT(false);
		/* in release build, carry on: we'll just miss per-iter refs */
	}
	return DR_EMIT_DEFAULT;
}

dr_emit_flags_t memory_inst::event_app_instruction(void *drcontext, void *tag, instrlist_t *bb,
	instr_t *instr, bool for_trace,
	bool translating, void *user_data) {

	if (!instr_is_app(instr))
		return DR_EMIT_DEFAULT;
	if (!instr_reads_memory(instr) && !instr_writes_memory(instr))
		return DR_EMIT_DEFAULT;

	// TODO: Tune this, currently extremely inefficient
	app_pc address = instr_get_app_pc(instr);
	uint opcode = instr_get_opcode(instr);
	instr_t *nxt = instr_get_next(instr);
	dr_insert_clean_call(drcontext, bb, nxt, memory_inst::at_access_mem,
		false    /*don't need to save fp state*/,
		2        /* 2 parameters */,
		/* opcode is 1st parameter */
		OPND_CREATE_INT32(opcode),
		/* address is 2nd parameter */
		OPND_CREATE_INTPTR(address));

	return DR_EMIT_DEFAULT;
}
