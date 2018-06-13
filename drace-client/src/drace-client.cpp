// DynamoRIO client for Race-Detection

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drwrap.h>

#include <atomic>
#include <iostream>

#include "libdrace-client.h"
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

	// Register Events
	dr_register_exit_event(event_exit);
	drmgr_register_thread_init_event(event_thread_init);
	drmgr_register_thread_exit_event(event_thread_exit);
	drmgr_register_module_load_event(module_load_event);

//	if (!drmgr_register_thread_init_event(event_thread_init) ||
//		!drmgr_register_thread_exit_event(event_thread_exit) ||
//		!drmgr_register_bb_app2app_event(event_bb_app2app, NULL) ||
//		!drmgr_register_bb_instrumentation_event(NULL /*analysis_func*/,
//			event_app_instruction,
//			NULL))
//		DR_ASSERT(false);

	// TODO: Thread Local Storage
	//tls_idx = drmgr_register_tls_field();
	//DR_ASSERT(tls_idx != -1);
	///* The TLS field provided by DR cannot be directly accessed from the code cache.
	//* For better performance, we allocate raw TLS so that we can directly
	//* access and update it with a single instruction.
	//*/
	//if (!dr_raw_tls_calloc(&tls_seg, &tls_offs, MEMTRACE_TLS_COUNT, 0))
	//	DR_ASSERT(false);
}

static void event_exit(void)
{
	// TODO: End epoch and print stats
	std::cout << "BB exit, Num Threads: " << num_threads_active << std::endl;
}

static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating)
{
	// TODO: inst events:
	// read, write
	// barrier (dmb)
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
	++num_threads_active;
	std::cout << "<< Thread started " << std::endl;
}

static void event_thread_exit(void *drcontext)
{
	// TODO: Cleanup and quit shadow thread
	// If only one thead is running, disable detector
	--num_threads_active;
	std::cout << "<< Thread exited " << std::endl;
}


static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
	// bind function wrappers
	wrap_mutex_acquire(mod);
	wrap_mutex_release(mod);
}