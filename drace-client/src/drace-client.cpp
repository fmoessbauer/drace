// DynamoRIO client for Race-Detection

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drwrap.h>
#include <drutil.h>

#include <atomic>
#include <string>

#include "drace-client.h"
#include "memory-instr.h"
#include "function-wrapper.h"
#include "module-tracker.h"

#include <detector_if.h>

/*
* Thread local storage metadata has to be globally accessable
*/
reg_id_t tls_seg;
uint     tls_offs;
int      tls_idx;

std::atomic<uint> runtime_tid{ 0 };

/* Runtime parameters */
params_t params;

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	/* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
	drreg_options_t ops = { sizeof(ops), 3, false };

	dr_set_client_name("Race-Detection Client 'drace'",
		"https://code.siemens.com/felix.moessbauer.ext/drace/issues");

	dr_enable_console_printing();

	// Parse runtime arguments and print generated configuration
	parse_args(argc, argv);
	print_config();

	// Init DRMGR, Reserve registers
	if (!drmgr_init() || !drwrap_init() || drreg_init(&ops) != DRREG_SUCCESS)
		DR_ASSERT(false);

	// performance tuning
	drwrap_set_global_flags((drwrap_global_flags_t)(DRWRAP_NO_FRILLS | DRWRAP_FAST_CLEANCALLS));

	// Register Events
	dr_register_exit_event(event_exit);
	drmgr_register_thread_init_event(event_thread_init);
	drmgr_register_thread_exit_event(event_thread_exit);

	// Setup Module Tracking
	DR_ASSERT(module_tracker::register_events());
	module_tracker::init();

	// Setup Memory Tracing
	DR_ASSERT(memory_inst::register_events());
	memory_inst::init();

	// Initialize Detector
	detector::init(argc, argv);
}

static void event_exit()
{
	if (!drmgr_register_thread_init_event(event_thread_init) ||
		!drmgr_register_thread_exit_event(event_thread_exit))
		DR_ASSERT(false);

	// Cleanup Module Tracker
	module_tracker::finalize();

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
	// assume that the first event comes from the runtime thread
	if (0 == num_threads_active) {
		runtime_tid = tid;
		dr_printf("<< [%i] Runtime Thread tagged\n", tid);
	}
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

static void parse_args(int argc, const char ** argv) {
	params.sampling_rate = 1;

	int processed = 1;
	while (processed < argc) {
		if (strncmp(argv[processed],"-s",16)==0) {
			params.sampling_rate = std::stoi(argv[processed + 1]);
			processed+=2;
		}
		else {
			// unknown argument skip as probably for detector
			++processed;
		}
	}
}

static void print_config() {
	dr_printf("< Runtime Configuration:\n"
		      "< Sampling Rate: %i\n", params.sampling_rate);
}
