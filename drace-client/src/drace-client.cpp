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
#include "Module.h"
#include "symbols.h"
#include "statistics.h"
#include "sink/hr-text.h"
#ifdef XML_EXPORTER
#include "sink/valkyrie.h"
#endif

#include <detector/detector_if.h>

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
	using namespace drace;

	dr_set_client_name("Race-Detection Client 'drace'",
		"https://code.siemens.com/felix.moessbauer.ext/drace/issues");

	dr_enable_console_printing();

	// Parse runtime arguments and print generated configuration
	drace::parse_args(argc, argv);
	drace::print_config();

	bool success = config.loadfile(params.config_file);
	if (!success) {
		LOG_ERROR(-1, "error loading config file");
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

	auto symbol_table = std::make_shared<Symbols>();

	// Setup Module Tracking
	module_tracker = std::make_unique<drace::module::Tracker>(symbol_table);

	// Setup Memory Tracing
	memory_tracker = std::make_unique<MemoryTracker>();

	// Setup Race Collector and bind lookup function
	race_collector = std::make_unique<RaceCollector>(
		params.delayed_sym_lookup,
		symbol_table);

	// Initialize Detector
	detector::init(argc, argv, race_collector_add_race);

	LOG_INFO(-1, "application pid: %i", dr_get_process_id());
	
	// if we try to access a non-existing SHM,
	// DR will spuriously fail some time later
	if (params.extctrl) {
		DR_ASSERT(MSR::connect(), "MSR not available");
	}

	app_start = std::chrono::system_clock::now();
}

namespace drace {
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
		stats.reset();

		funwrap::finalize();

		// Cleanup DR extensions
		drutil_exit();
		drmgr_exit();

		// Finalize Detector
		detector::finalize();

		dr_mutex_destroy(th_mutex);
		dr_rwlock_destroy(tls_rw_mutex);

		LOG_INFO(-1, "DR exit");
	}

	static void event_thread_init(void *drcontext)
	{
		using namespace drace;
		thread_id_t tid = dr_get_thread_id(drcontext);
		// assume that the first event comes from the runtime thread
		unsigned empty_tid = 0;
		if (runtime_tid.compare_exchange_weak(empty_tid, tid, std::memory_order_relaxed)) {
			LOG_INFO(tid, "Runtime Thread tagged");
		}
		num_threads_active.fetch_add(1, std::memory_order_relaxed);

		memory_tracker->event_thread_init(drcontext);
		LOG_INFO(tid, "Thread started");
	}

	static void event_thread_exit(void *drcontext)
	{
		using namespace drace;
		thread_id_t tid = dr_get_thread_id(drcontext);
		num_threads_active.fetch_sub(1, std::memory_order_relaxed);

		memory_tracker->event_thread_exit(drcontext);

		LOG_INFO(tid, "Thread exited");
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
			else if (strncmp(argv[processed], "--excl-traces", 16) == 0) {
				params.excl_traces = true;
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
			else if (strncmp(argv[processed], "--extctrl", 16) == 0) {
				params.extctrl = true;
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
			"< Exclude Traces:\t%s\n"
			"< Yield on Event:\t%s\n"
			"< Exclude Master:\t%s\n"
			"< Delayed Sym Lookup:\t%s\n"
			"< Fast Mode:\t\t%s\n"
			"< Config File:\t\t%s\n"
			"< Output File:\t\t%s\n"
			"< XML File:\t\t%s\n"
			"< Stack-Size:\t\t%i\n"
			"< External Ctrl:\t%s\n"
			"< Private Caches:\t%s\n",
			params.sampling_rate,
			params.instr_rate,
			params.lossy ? "ON" : "OFF",
			params.lossy_flush ? "ON" : "OFF",
			params.excl_traces ? "ON" : "OFF",
			params.yield_on_evt ? "ON" : "OFF",
			params.exclude_master ? "ON" : "OFF",
			params.delayed_sym_lookup ? "ON" : "OFF",
			params.fastmode ? "ON" : "OFF",
			params.config_file.c_str(),
			params.out_file != "" ? params.out_file.c_str() : "OFF",
			params.xml_file != "" ? params.xml_file.c_str() : "OFF",
			params.stack_size,
			params.extctrl ? "ON" : "OFF",
			dr_using_all_private_caches() ? "ON" : "OFF");
	}

	static void generate_summary() {
		using namespace drace;
		race_collector->resolve_all();

		if (params.out_file != "") {
			std::ofstream races_hr_file(params.out_file, std::ofstream::out);
			sink::HRText<std::ofstream> hr_sink(races_hr_file);
			hr_sink.process_all(race_collector->get_races());
			races_hr_file.close();
		}

#ifdef XML_EXPORTER
		// Write XML output
		if (params.xml_file != "") {
			std::ofstream races_xml_file(params.xml_file, std::ofstream::out);
			sink::Valkyrie<std::ofstream> v_sink(races_xml_file,
				params.argc, params.argv, dr_get_application_name(),
				app_start, app_stop);
			v_sink.process_all(race_collector->get_races());
		}
#endif
	}

} // namespace drace