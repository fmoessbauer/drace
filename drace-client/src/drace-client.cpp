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

#include "globals.h"
#include "drace-client.h"
#include "race-collector.h"
#include "memory-tracker.h"
#include "function-wrapper.h"
#include "module-tracker.h"
#include "symbols.h"
#include "statistics.h"
#include "sink/hr-text.h"
#include "sink/valkyrie.h"

#include <detector_if.h>

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	dr_set_client_name("Race-Detection Client 'drace'",
		"https://code.siemens.com/felix.moessbauer.ext/drace/issues");

	dr_enable_console_printing();

	// Parse runtime arguments and print generated configuration
	parse_args(argc, argv);
	print_config();

	bool success = config.loadfile(params.config_file);
	if (!success) {
		dr_printf("Error loading config file\n");
		dr_flush_file(stdout);
		exit(1);
	}

	TLS_buckets.reserve(128);

	th_mutex = dr_mutex_create();
	tls_rw_mutex = dr_rwlock_create();

	// Init DRMGR, Reserve registers
	if (!drmgr_init() ||
		!drutil_init())
		DR_ASSERT(false);

	// Register Events
	dr_register_exit_event(event_exit);
	drmgr_register_thread_init_event(event_thread_init);
	drmgr_register_thread_exit_event(event_thread_exit);

	// Setup Statistics Collector
	stats = std::make_unique<Statistics>(0);

	// Setup Function Wrapper
	DR_ASSERT(funwrap::init());

	symbol_table = std::make_shared<Symbols>();

	// Setup Module Tracking
	module_tracker = std::make_unique<ModuleTracker>(symbol_table);

	// Setup Memory Tracing
	memory_tracker = std::make_unique<MemoryTracker>();

	// Setup Race Collector and bind lookup function
	race_collector = std::make_unique<RaceCollector>(
		params.delayed_sym_lookup,
		symbol_table);

	// Initialize Detector
	detector::init(argc, argv, race_collector_add_race);

	app_start = std::chrono::system_clock::now();
}

static void event_exit()
{
	app_stop = std::chrono::system_clock::now();

	if (!drmgr_unregister_thread_init_event(event_thread_init) ||
		!drmgr_unregister_thread_exit_event(event_thread_exit))
		DR_ASSERT(false);

	// Generate summary while information is still present
	generate_summary();
	stats->print_summary(std::cout);

	// Cleanup all drace modules
	module_tracker.reset();
	memory_tracker.reset();
	symbol_table.reset();
	stats.reset();

	funwrap::finalize();

	// Cleanup DR extensions
	drutil_exit();
	drmgr_exit();

	// Finalize Detector
	detector::finalize();

	dr_mutex_destroy(th_mutex);
	dr_rwlock_destroy(tls_rw_mutex);

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

	memory_tracker->event_thread_init(drcontext);

	dr_printf("<< [%.5i] Thread started\n", tid);
}

static void event_thread_exit(void *drcontext)
{
	thread_id_t tid = dr_get_thread_id(drcontext);
	num_threads_active.fetch_sub(1, std::memory_order_relaxed);

	memory_tracker->event_thread_exit(drcontext);

	dr_printf("<< [%.5i] Thread exited\n", tid);
}

static void parse_args(int argc, const char ** argv) {
	params.argc = argc;
	params.argv = argv;

	int processed = 1;
	while (processed < argc) {
		if (strncmp(argv[processed], "-s", 16) == 0) {
			params.sampling_rate = std::stoi(argv[++processed]);
		}
		else if (strncmp(argv[processed], "-i", 16) == 0) {
			params.instr_rate = std::stoi(argv[++processed]);
		}
		else if (strncmp(argv[processed], "-c", 16) == 0) {
			params.config_file = argv[++processed];
		}
		else if (strncmp(argv[processed], "--lossy", 16) == 0) {
			params.lossy = true;
		}
		else if (strncmp(argv[processed], "--lossy-flush", 16) == 0) {
			params.lossy_flush = true;
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
		else if (strncmp(argv[processed], "--fast-mode", 16) == 0) {
			params.fastmode = true;
		}
		else if (strncmp(argv[processed], "--xml-file", 16) == 0) {
			params.xml_file = argv[++processed];
		}
		else if (strncmp(argv[processed], "--out-file", 16) == 0) {
			params.out_file = argv[++processed];
		}
		else if (strncmp(argv[processed], "--stacksz", 16) == 0) {
			params.stack_size = std::stoi(argv[++processed]);
		}
		// unknown argument skip as probably for detector
		++processed;
	}
}

static void print_config() {
	dr_printf(
		"< Runtime Configuration:\n"
		"< Sampling Rate:\t%i\n"
		"< Instr. Rate:\t\t%i\n"
		"< Lossy:\t\t%s\n"
		"< Lossy-Flush:\t\t%s\n"
		"< Yield on Event:\t%s\n"
		"< Exclude Master:\t%s\n"
		"< Delayed Sym Lookup:\t%s\n"
		"< Fast Mode:\t\t%s\n"
		"< Config File:\t\t%s\n"
		"< Output File:\t\t%s\n"
		"< XML File:\t\t%s\n"
		"< Stack-Size:\t\t%i\n"
		"< Private Caches:\t%s\n",
		params.sampling_rate,
		params.instr_rate,
		params.lossy ? "ON" : "OFF",
		params.lossy_flush ? "ON" : "OFF",
		params.yield_on_evt ? "ON" : "OFF",
		params.exclude_master ? "ON" : "OFF",
		params.delayed_sym_lookup ? "ON" : "OFF",
		params.fastmode ? "ON" : "OFF",
		params.config_file.c_str(),
		params.out_file != "" ? params.out_file : "OFF",
		params.xml_file != "" ? params.xml_file : "OFF",
		params.stack_size,
		dr_using_all_private_caches() ? "ON" : "OFF");
}

static void generate_summary() {
	race_collector->resolve_all();
	const char * app_name = dr_get_application_name();

	if (params.out_file != "") {
		std::ofstream races_hr_file(params.out_file, std::ofstream::out);
		sink::HRText<std::ofstream> hr_sink(races_hr_file);
		hr_sink.process_all(race_collector->get_races());
		races_hr_file.close();
	}
	
	// Write XML output
	if (params.xml_file != "") {
		std::ofstream races_xml_file(params.xml_file, std::ofstream::out);
		sink::Valkyrie<std::ofstream> v_sink(races_xml_file,
			params.argc, params.argv, dr_get_application_name(),
			app_start, app_stop);
		v_sink.process_all(race_collector->get_races());
	}
}