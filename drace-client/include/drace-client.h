#pragma once
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

namespace drace {
	// Events
	static void event_exit(void);
	static void event_thread_init(void *drcontext);
	static void event_thread_exit(void *drcontext);

	// Runtime Configuration
	static void parse_args(int argc, const char **argv);
	static void print_config();

	static void generate_summary();
}
