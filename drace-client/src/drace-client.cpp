/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

 /**
  \brief DynamoRIO client for Race-Detection
  */

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
#include "MSR.h"

#include <clipp.h>
#include <detector/detector_if.h>
#include <version/version.h>

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[])
{
    using namespace drace;

    dr_set_client_name("Race-Detection Tool 'DRace'",
        "https://github.com/siemens/drace/issues");

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
    LOG_NOTICE(-1, "size of per_thread_t %i bytes", sizeof(per_thread_t));

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
    // register the console printer
    race_collector->register_sink(
        std::make_shared<sink::HRText>(drace::log_target)
    );

    // Initialize Detector
    detector::init(argc, argv, race_collector_add_race);

    LOG_INFO(-1, "application pid: %i", dr_get_process_id());

    // if we try to access a non-existing SHM,
    // DR will spuriously fail some time later
    if (params.extctrl) {
        if (!MSR::connect()) {
            LOG_ERROR(-1, "MSR not available (required for --extctrl)");
            dr_abort();
        }
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
        stats->print_summary(drace::log_target);

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

        LOG_INFO(-1, "DRace exit");

        if (drace::log_requires_close)
            dr_close_file(drace::log_target);
    }

    static void event_thread_init(void *drcontext)
    {
        using namespace drace;
        thread_id_t tid = dr_get_thread_id(drcontext);
        // assume that the first event comes from the runtime thread
        unsigned empty_tid = 0;
        if (runtime_tid.compare_exchange_weak(empty_tid, (unsigned)tid, std::memory_order_relaxed)) {
            LOG_INFO(tid, "Runtime Thread tagged");
        }
        num_threads_active.fetch_add(1, std::memory_order_relaxed);

        memory_tracker->event_thread_init(drcontext);
        LOG_INFO(tid, "Thread started");
    }

    static void event_thread_exit(void *drcontext)
    {
        using namespace drace;
        num_threads_active.fetch_sub(1, std::memory_order_relaxed);

        memory_tracker->event_thread_exit(drcontext);

        LOG_INFO(-1, "Thread exited");
    }

    static void parse_args(int argc, const char ** argv) {
        params.argc = argc;
        params.argv = argv;

        bool display_help = false;
        auto drace_cli = clipp::group(
            (clipp::option("-c", "--config") & clipp::value("config", params.config_file)) % ("config file (default: " + params.config_file + ")"),
            (
            (clipp::option("-s", "--sample-rate") & clipp::integer("sample-rate", params.sampling_rate)) % "sample each nth instruction (default: no sampling)",
                (clipp::option("-i", "--instr-rate")  & clipp::integer("instr-rate", params.instr_rate)) % "instrument each nth instruction (default: no sampling)"
                ) % "sampling options",
                (
            (clipp::option("--lossy").set(params.lossy) % "dynamically exclude fragments using lossy counting") &
                    (clipp::option("--lossy-flush").set(params.lossy_flush) % "de-instrument flushed segments (only with --lossy)"),
                    clipp::option("--excl-traces").set(params.excl_traces) % "exclude dynamorio traces",
                    clipp::option("--excl-stack").set(params.excl_stack) % "exclude stack accesses",
                    clipp::option("--excl-master").set(params.exclude_master) % "exclude first thread"
                    ) % "analysis scope",
                    (clipp::option("--stacksz") & clipp::integer("stacksz", params.stack_size)) %
            ("size of callstack used for race-detection (must be in [1,16], default: " + std::to_string(params.stack_size) + ")"),
            clipp::option("--no-annotations").set(params.annotations, false) % "disable code annotation support",
            clipp::option("--delay-syms").set(params.delayed_sym_lookup) % "perform symbol lookup after application shutdown",
            clipp::option("--sync-mode").set(params.fastmode, false) % "flush all buffers on a sync event (instead of participating only)",
            clipp::option("--fast-mode").set(params.fastmode) % "DEPRECATED: inverse of sync-mode",
            (clipp::option("--suplevel") & clipp::integer("level", params.suppression_level)) % "suppress similar races (0=detector-default, 1=unique top-of-callstack entry, default: 1)",
            (
#ifndef DRACE_USE_LEGACY_API
            (clipp::option("--xml-file", "-x") & clipp::value("filename", params.xml_file)) % "log races in valkyries xml format in this file",
#endif
                (clipp::option("--out-file", "-o") & clipp::value("filename", params.out_file)) % "log races in human readable format in this file"
                ) % "data race reporting",
                (clipp::option("--logfile", "-l") & clipp::value("filename", params.logfile)) % "write all logs to this file (can be null, stdout, stderr, or filename)",
            clipp::option("--extctrl").set(params.extctrl) % "use second process for symbol lookup and state-controlling (required for Dotnet)",
            // for testing reasons only. Abort execution after the first race was detected
            clipp::option("--brkonrace").set(params.break_on_race) % "abort execution after first race is found (for testing purpose only)",
            (clipp::option("--version")([]() {
            std::cout << "DRace, a dynamic data race detector\nVersion: " << DRACE_BUILD_VERSION << "\n"
                << "Hash:    " << DRACE_BUILD_HASH << std::endl;
#ifdef DRACE_USE_LEGACY_API
            std::cout << "Note:    Windows 7 compatible build with limited feature set" << std::endl;
#endif
            dr_abort(); }) % "display version information"),
            clipp::option("-h", "--usage").set(display_help) % "display help"
                );
        auto detector_cli = clipp::group(
            // we just name the options here to provide a well-defined cli.
            // The detector parses the argv itself
            clipp::option("--heap-only") % "only analyze heap memory"
        );
        auto cli = (
            (drace_cli % "DRace Options"),
            (detector_cli % ("Detector (" + detector::name() + ") Options"))
            );

        if (!clipp::parse(argc, (char**)argv, cli) || display_help) {
            std::stringstream out;
            out << clipp::make_man_page(cli, util::basename(argv[0])) << std::endl;
            std::string outstr = out.str();
            // dr_fprintf does not print anything if input buffer
            // is too large hence, chunk it
            const auto chunks = util::split(outstr, "\n");
            for (const auto & chunk : chunks) {
                dr_write_file(STDERR, chunk.c_str(), chunk.size());
                dr_write_file(STDERR, "\n", 1);
            }
            dr_abort();
        }

        // setup logging target
        if (params.logfile == "null")
            drace::log_target = nullptr;
        else if (params.logfile == "stdout")
            drace::log_target = (FILE*)dr_get_stdout_file();
        else if (params.logfile == "stderr")
            drace::log_target = (FILE*)dr_get_stderr_file();
        else if (params.logfile != "") { // filename
            if ((drace::log_target = (FILE*)dr_open_file(params.logfile.c_str(), DR_FILE_WRITE_OVERWRITE)) != INVALID_FILE)
                drace::log_requires_close = true;
        }
        else
            drace::log_target = nullptr;
    }

    static void print_config() {
        dr_fprintf(drace::log_target,
            "< Runtime Configuration:\n"
            "< Sampling Rate:\t%i\n"
            "< Instr. Rate:\t\t%i\n"
            "< Lossy:\t\t%s\n"
            "< Lossy-Flush:\t\t%s\n"
            "< Exclude Traces:\t%s\n"
            "< Exclude Stack:\t%s\n"
            "< Exclude Master:\t%s\n"
            "< Annotation Sup.:\t%s\n"
            "< Delayed Sym Lookup:\t%s\n"
            "< Fast Mode:\t\t%s\n"
            "< Config File:\t\t%s\n"
            "< Output File:\t\t%s\n"
            "< XML File:\t\t%s\n"
            "< Stack-Size:\t\t%i\n"
            "< External Ctrl:\t%s\n"
            "< Log Target:\t\t%s\n"
            "< Private Caches:\t%s\n",
            params.sampling_rate,
            params.instr_rate,
            params.lossy ? "ON" : "OFF",
            params.lossy_flush ? "ON" : "OFF",
            params.excl_traces ? "ON" : "OFF",
            params.excl_stack ? "ON" : "OFF",
            params.exclude_master ? "ON" : "OFF",
            params.annotations ? "ON" : "OFF",
            params.delayed_sym_lookup ? "ON" : "OFF",
            params.fastmode ? "ON" : "OFF",
            params.config_file.c_str(),
            params.out_file != "" ? params.out_file.c_str() : "OFF",
#ifdef DRACE_USE_LEGACY_API
            "Unsupported (legacy build)",
#else
            params.xml_file != "" ? params.xml_file.c_str() : "OFF",
#endif
            params.stack_size,
            params.extctrl ? "ON" : "OFF",
            params.logfile,
            dr_using_all_private_caches() ? "ON" : "OFF");
    }

    static void generate_summary() {
        using namespace drace;
        race_collector->resolve_all();

        if (params.out_file != "") {
            FILE * racereport = (FILE*)dr_open_file(params.out_file.c_str(), DR_FILE_WRITE_OVERWRITE);
            if (racereport != INVALID_FILE) {
                sink::HRText hr_sink(racereport);
                hr_sink.process_all(race_collector->get_races());
                dr_close_file(racereport);
            }
            else {
                LOG_ERROR(-1, "Could not open race-report file: %c", params.out_file.c_str());
            }
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
