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

#include <tsan-custom.h>

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	num_threads_active = 0;
	/* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
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

	// Initialize Detector
	// TODO: Fill in final data
	__tsan_init_simple(reportRaceCallBack, (void*)0xABCD);
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
	// kills program, hence skip
	//__tsan_fini();

	dr_printf("< DR Exit\n");
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
	funwrap::wrap_allocators(mod);
	funwrap::wrap_deallocs(mod);
	funwrap::wrap_main(mod);
}

// Detector Stuff
void reportRaceCallBack(__tsan_race_info* raceInfo, void* callback_parameter) {
	std::cout << "DATA Race " << callback_parameter << ::std::endl;


	for (int i = 0; i != 2; ++i) {
		__tsan_race_info_access* race_info_ac = NULL;

		if (i == 0) {
			race_info_ac = raceInfo->access1;
			std::cout << " Access 1 tid: " << race_info_ac->user_id << " ";

		}
		else {
			race_info_ac = raceInfo->access2;
			std::cout << " Access 2 tid: " << race_info_ac->user_id << " ";
		}
		std::cout << (race_info_ac->write == 1 ? "write" : "read") << " to/from " << race_info_ac->accessed_memory << " with size " << race_info_ac->size << ". Stack(Size " << race_info_ac->stack_trace_size << ")" << "Type: " << race_info_ac->type << " :" << ::std::endl;

		for (int i = 0; i != race_info_ac->stack_trace_size; ++i) {
			std::cout << ((void**)race_info_ac->stack_trace)[i] << std::endl;
		}
	}
}
