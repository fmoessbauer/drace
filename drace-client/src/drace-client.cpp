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

#include <detector_if.h>

/*
* Thread local storage metadata has to be globally accessable
*/
reg_id_t tls_seg;
uint     tls_offs;
int      tls_idx;

/* Runtime parameters */
int      sampling_rate = 1;

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	/* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
	drreg_options_t ops = { sizeof(ops), 3, false };

	dr_set_client_name("Race-Detection Client 'drace'",
		"https://code.siemens.com/felix.moessbauer.ext/drace/issues");

	dr_enable_console_printing();

	// TODO: Read console parameters

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
	memory_inst::register_tls();

	// Initialize Detector
	detector::init();
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

	// Finalize Detector
	detector::finalize();

	dr_printf("< DR Exit\n");
}

static void event_thread_init(void *drcontext)
{
	thread_id_t tid = dr_get_thread_id(drcontext);
	++num_threads_active;

	// TODO: Try to get parent threadid
	detector::fork(0, tid);

	dr_printf("<< [%i] Thread started\n", tid);
}

static void event_thread_exit(void *drcontext)
{
	thread_id_t tid = dr_get_thread_id(drcontext);
	--num_threads_active;

	// TODO: Try to get parent threadid
	detector::join(0, tid);

	dr_printf("<< [%i] Thread exited\n", tid);
}


static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
	// bind function wrappers
	funwrap::wrap_mutex_acquire(mod);
	funwrap::wrap_mutex_release(mod);
	funwrap::wrap_allocators(mod);
	funwrap::wrap_deallocs(mod);
	funwrap::wrap_main(mod);
}
