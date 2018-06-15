// DynamoRIO client for Race-Detection

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drwrap.h>
#include <drutil.h>

#include <atomic>
#include <iostream>

#include "drace-client.h"
#include "memory-instr.h"
#include "function-wrapper.h"

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	num_threads_active = 0;
	drreg_options_t ops = { sizeof(ops), 3, false };

	dr_set_client_name("Race-Detection Client 'drace'",
		"https://code.siemens.com/felix.moessbauer.ext/drace/issues");

	dr_enable_console_printing();

	// Init DRMGR, Reserve registers
	if (!drmgr_init() || !drwrap_init() || drreg_init(&ops) != DRREG_SUCCESS)
		DR_ASSERT(false);

	// performance tuning
	drwrap_set_global_flags((drwrap_global_flags_t)(DRWRAP_NO_FRILLS | DRWRAP_FAST_CLEANCALLS));

	// Register Events
	dr_register_exit_event(event_exit);
	drmgr_register_thread_init_event(event_thread_init);
	drmgr_register_thread_exit_event(event_thread_exit);
	drmgr_register_module_load_event(module_load_event);

	// Setup Memory Tracing
	DR_ASSERT(memory_inst::register_events());
	memory_inst::allocate_tls();
}

static void event_exit()
{
	if (!drmgr_register_thread_init_event(event_thread_init) ||
		!drmgr_register_thread_exit_event(event_thread_exit) ||
		!drmgr_register_module_load_event(module_load_event))
		DR_ASSERT(false);

	// Cleanup Memory Tracing
	memory_inst::finalize();

	drutil_exit();
	drmgr_exit();

	dr_printf("< DR Exit\n");
}

static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
{
	// TODO: inst events:
	// read, write
	// barrier (dmb) <- only for ARM
	// aqu,rel   <- handled using function wrapping
	// fork,join <- handled by thread events

	//instr_reads_memory, instr_writes_memory
	// call __gthrw_pthread_mutex_lock(pthread_mutex_t*)
	// call __gthrw_pthread_mutex_unlock(pthread_mutex_t*)

	// semget
	// semop

	//int i;

	//if (!instr_is_app(instr))
	//	return DR_EMIT_DEFAULT;
	//if (!instr_reads_memory(instr) && !instr_writes_memory(instr))
	//	return DR_EMIT_DEFAULT;

	///* insert code to add an entry for app instruction */
	////instrument_instr(drcontext, bb, instr);

	/* insert code to add an entry for each memory reference opnd */
	//for (int i = 0; i < instr_num_srcs(instr); i++) {
	//	if (opnd_is_memory_reference(instr_get_src(instr, i)))
	//		//instrument_mem(drcontext, bb, instr, instr_get_src(instr, i), false);
	//}

	//for (i = 0; i < instr_num_dsts(instr); i++) {
	//	if (opnd_is_memory_reference(instr_get_dst(instr, i)))
	//		//instrument_mem(drcontext, bb, instr, instr_get_dst(instr, i), true);
	//}

	return DR_EMIT_DEFAULT;
}

static void event_thread_init(void *drcontext)
{
	// TODO: Start shadow thread for each app thread
	// If only one thead is running, disable detector
	thread_id_t tid = dr_get_thread_id(drcontext);
	++num_threads_active;
	dr_printf("<< [%i] Thread started\n", tid);
}

static void event_thread_exit(void *drcontext)
{
	// TODO: Cleanup and quit shadow thread
	// If only one thead is running, disable detector
	thread_id_t tid = dr_get_thread_id(drcontext);
	--num_threads_active;
	dr_printf("<< [%i] Thread exited\n", tid);
}


static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
	// bind function wrappers
	funwrap::wrap_mutex_acquire(mod);
	funwrap::wrap_mutex_release(mod);
}