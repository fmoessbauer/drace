#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2020 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

namespace drace {
// Events
/// callback on DynamoRio exit
static void event_exit(void);
/// global callback for thread init in target application
static void event_thread_init(void *drcontext);
/// global callback for thread exit in target application
static void event_thread_exit(void *drcontext);

/// load and initialize detector
static void register_detector(int argc, const char **argv,
                              const std::string &detector_name);

/// register sinks for race reporting
static void register_report_sinks();

/// generate data-race summary
static void generate_summary();
}  // namespace drace
