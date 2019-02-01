#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
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
