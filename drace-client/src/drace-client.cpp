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
#include "stack-demangler.h"
#include "symbols.h"

#include <detector_if.h>

/*
* Thread local storage metadata has to be globally accessable
*/
reg_id_t tls_seg;
uint     tls_offs;
int      tls_idx;

void *th_mutex;

std::atomic<uint> runtime_tid{ 0 };
std::unordered_map<thread_id_t, per_thread_t*> TLS_buckets;

/* Runtime parameters */
params_t params;

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	dr_set_client_name("Race-Detection Client 'drace'",
		"https://code.siemens.com/felix.moessbauer.ext/drace/issues");

	dr_enable_console_printing();

	// Parse runtime arguments and print generated configuration
	parse_args(argc, argv);
	print_config();

	TLS_buckets.reserve(128);

	th_mutex = dr_mutex_create();

	// Init DRMGR, Reserve registers
	if (!drmgr_init() ||
		!drutil_init())
		DR_ASSERT(false);

	// Register Events
	dr_register_exit_event(event_exit);
	drmgr_register_thread_init_event(event_thread_init);
	drmgr_register_thread_exit_event(event_thread_exit);

	// Setup Function Wrapper
	DR_ASSERT(funwrap::init());

	// Setup Module Tracking
	module_tracker::init();
	DR_ASSERT(module_tracker::register_events());

	// Setup Memory Tracing
	memory_inst::init();
	DR_ASSERT(memory_inst::register_events());

	// Setup Symbol Access Lib
	symbols::init();

	// Initialize Detector
	if (!params.delayed_sym_lookup) {
		detector::init(argc, argv, stack_demangler::demangle);
	}
	else {
		detector::init(argc, argv, nullptr);
	}
}

static void event_exit()
{
	if (!drmgr_register_thread_init_event(event_thread_init) ||
		!drmgr_register_thread_exit_event(event_thread_exit))
		DR_ASSERT(false);

	// Cleanup all drace modules
	module_tracker::finalize();
	memory_inst::finalize();
	symbols::finalize();
	funwrap::finalize();

	// Cleanup DR extensions
	drutil_exit();
	drmgr_exit();

	// Finalize Detector
	detector::finalize();

	dr_mutex_destroy(th_mutex);

	dr_printf("< DR Exit\n");
}

static void event_thread_init(void *drcontext)
{
	thread_id_t tid = dr_get_thread_id(drcontext);
	// assume that the first event comes from the runtime thread
	if (0 == num_threads_active) {
		runtime_tid = tid;
		dr_printf("<< [%.5i] Runtime Thread tagged\n", tid);
	}
	++num_threads_active;

	// TODO: Try to get parent threadid
	detector::fork(0, tid);

	dr_printf("<< [%.5i] Thread started\n", tid);
}

static void event_thread_exit(void *drcontext)
{
	thread_id_t tid = dr_get_thread_id(drcontext);
	--num_threads_active;

	// TODO: Try to get parent threadid
	detector::join(0, tid);

	dr_printf("<< [%.5i] Thread exited\n", tid);
}

static void parse_args(int argc, const char ** argv) {
	int processed = 1;
	while (processed < argc) {
		if (strncmp(argv[processed],"-s",16)==0) {
			params.sampling_rate = std::stoi(argv[processed + 1]);
			++processed;
		}
		else if (strncmp(argv[processed], "--freq-only", 16) == 0) {
			params.frequent_only = true;
		}
		else if (strncmp(argv[processed], "--excl-master", 16) == 0) {
			params.exclude_master = true;
		}
		else if (strncmp(argv[processed], "--yield-on-evt", 16) == 0) {
			params.yield_on_evt = true;
		}
		else if (strncmp(argv[processed], "--delayed-syms", 16) == 0) {
			params.delayed_sym_lookup = true;
		}
		// unknown argument skip as probably for detector
		++processed;
	}
}

static void print_config() {
	dr_printf(
		"< Runtime Configuration:\n"
		"< Sampling Rate:\t%i\n"
		"< Frequent Only:\t%s\n"
		"< Yield on Event:\t%s\n"
		"< Exclude Master:\t%s\n"
		"< Delayed Sym Lookup:\t%s\n",
		params.sampling_rate,
		params.frequent_only  ? "ON" : "OFF",
		params.yield_on_evt   ? "ON" : "OFF",
		params.exclude_master ? "ON" : "OFF",
		params.delayed_sym_lookup ? "ON" : "OFF");
}
