// DynamoRIO client for Race-Detection

#include <dr_api.h>
#include <drmgr.h>
#include <drreg.h>
#include <drwrap.h>
#include <drutil.h>

#include <atomic>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <functional>

#include "drace-client.h"
#include "memory-tracker.h"
#include "function-wrapper.h"
#include "module-tracker.h"
#include "stack-demangler.h"
#include "symbols.h"
#include "race-collector.h"

#include <detector_if.h>
/*
* Thread local storage metadata has to be globally accessable
*/
reg_id_t tls_seg;
uint     tls_offs;
int      tls_idx;

void *th_mutex;
void *th_start_mutex;

// Global Config Object
drace::Config config;

std::atomic<uint> runtime_tid{ 0 };
std::unordered_map<thread_id_t, per_thread_t*> TLS_buckets;
std::atomic<uint> last_th_start{ 0 };
std::atomic<bool> th_start_pending{ false };

std::string config_file("drace.ini");

std::unique_ptr<MemoryTracker> memory_tracker;
std::unique_ptr<ModuleTracker> module_tracker;
std::unique_ptr<Symbols> symbol_table;
std::unique_ptr<RaceCollector> race_collector;

/* Runtime parameters */
params_t params;

void generate_summary();

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	dr_set_client_name("Race-Detection Client 'drace'",
		"https://code.siemens.com/felix.moessbauer.ext/drace/issues");

	dr_enable_console_printing();

	// Parse runtime arguments and print generated configuration
	parse_args(argc, argv);
	print_config();

	bool success = config.loadfile(config_file);
	if (!success) {
		dr_printf("Error loading config file\n");
		dr_flush_file(stdout);
		exit(1);
	}

	TLS_buckets.reserve(128);

	th_mutex       = dr_mutex_create();
	th_start_mutex = dr_mutex_create();

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
	module_tracker = std::make_unique<ModuleTracker>();
	DR_ASSERT(module_tracker->register_events());

	// Setup Memory Tracing
	memory_tracker = std::make_unique<MemoryTracker>();
	DR_ASSERT(memory_tracker->register_events());

	symbol_table   = std::make_unique<Symbols>();
	race_collector = std::make_unique<RaceCollector>(params.delayed_sym_lookup, stack_demangler::demangle);

	// Initialize Detector
	detector::init(argc, argv, race_collector_add_race);
}

static void event_exit()
{
	if (!drmgr_register_thread_init_event(event_thread_init) ||
		!drmgr_register_thread_exit_event(event_thread_exit))
		DR_ASSERT(false);

	// Generate summary while information is still present
	generate_summary();

	// Cleanup all drace modules
	module_tracker.reset();
	memory_tracker.reset();
	symbol_table.reset();
	funwrap::finalize();

	// Cleanup DR extensions
	drutil_exit();
	drmgr_exit();

	// Finalize Detector
	detector::finalize();

	dr_mutex_destroy(th_mutex);
	dr_mutex_destroy(th_start_mutex);

	dr_printf("< DR Exit\n");
}

static void event_thread_init(void *drcontext)
{
	thread_id_t tid = dr_get_thread_id(drcontext);
	// assume that the first event comes from the runtime thread
	unsigned empty_tid = 0;
	if (runtime_tid.compare_exchange_weak(empty_tid, tid, std::memory_order_relaxed)) {
		dr_printf("<< [%.5i] Runtime Thread tagged\n", tid);
	}
	num_threads_active.fetch_add(1, std::memory_order_relaxed);

	// Detector fork event is executed in
	// memory-instr's thread init event

	dr_printf("<< [%.5i] Thread started\n", tid);
}

static void event_thread_exit(void *drcontext)
{
	thread_id_t tid = dr_get_thread_id(drcontext);
	num_threads_active.fetch_sub(1, std::memory_order_relaxed);

	// TODO: Try to get parent threadid
	detector::join(runtime_tid.load(std::memory_order_relaxed), tid, nullptr);

	dr_printf("<< [%.5i] Thread exited\n", tid);
}

/* Request a flush of all non-disabled threads.
*  This function is NOT-Threadsafe, hence use it only in a locked-state
*  Invariant: TLS_buckets is not modified
*/
void flush_all_threads(per_thread_t * data) {
	memory_tracker->process_buffer();
	// issue flushes
	for (auto td : TLS_buckets) {
		if (td.first != data->tid)
		{
			//printf("[%.5i] Flush thread [%i]\n", data->tid, td.first);
			// check if memory_order_relaxed is sufficient
			td.second->no_flush.store(false, std::memory_order_relaxed);
		}
	}
	// wait until all threads flushed
	// this is a hacky half-barrier implementation
	// and this might dead-lock if only one core is avaliable
	for (auto td : TLS_buckets) {
		// Flush thread given that:
		// 1. thread is not the calling thread
		// 2. thread is not disabled
		if (td.first != data->tid && td.second->enabled)
		{
			unsigned long waits = 0;
			// TODO: validate this!!!
			// Only the flush-variable has to be set atomically
			while (!data->no_flush.load(std::memory_order_relaxed)) {
				if (++waits > 10) {
					// avoid busy-waiting and blocking CPU if other thread did not flush
					// within given period
					dr_thread_yield();
				}
			}
		}
	}
}

static void parse_args(int argc, const char ** argv) {
	int processed = 1;
	while (processed < argc) {
		if (strncmp(argv[processed],"-s",16)==0) {
			params.sampling_rate = std::stoi(argv[++processed]);
		}
		else if (strncmp(argv[processed], "-c", 16) == 0) {
			config_file = argv[++processed];
		}
		// TODO: Delay to speedup startup
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
		else if (strncmp(argv[processed], "--xml-file", 16) == 0) {
			params.xml_file = argv[++processed];
		}
		else if (strncmp(argv[processed], "--out-file", 16) == 0) {
			params.out_file = argv[++processed];
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
		"< Delayed Sym Lookup:\t%s\n"
		"< Config File:\t\t%s\n"
		"< Output File:\t\t%s\n"
		"< XML File:\t\t%s\n",
		params.sampling_rate,
		params.frequent_only  ? "ON" : "OFF",
		params.yield_on_evt   ? "ON" : "OFF",
		params.exclude_master ? "ON" : "OFF",
		params.delayed_sym_lookup ? "ON" : "OFF",
		config_file.c_str(),
		params.out_file != "" ? params.out_file : "OFF",
		params.xml_file != "" ? params.xml_file : "OFF");
}

void generate_summary() {
	const char * app_name = dr_get_application_name();
	if (params.out_file != "") {
		std::ofstream races_hr_file(params.out_file, std::ofstream::out);
		race_collector->write_hr(races_hr_file);
		races_hr_file.close();
	}

	// Write XML output
	if (params.xml_file != "") {
		std::ofstream races_xml_file(params.xml_file, std::ofstream::out);
		race_collector->write_xml(races_xml_file);
		races_xml_file.close();
	}
}