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

#include <dr_api.h>
#include "ipc/SMData.h"

namespace drace {
	/** Provides routines to perform communication with MSR */
	class MSR {
	public:
		/** Wait using a heart_beat until the result is returned */
		static void wait_heart_beat();
		static bool connect();
		static bool attach(const module_data_t * mod);
		static bool request_symbols(const module_data_t * mod);
		static void unload_symbols(app_pc mod_start);

		static ::ipc::SymbolInfo lookup_address(app_pc pc);
		static ::ipc::SymbolResponse search_symbol(const module_data_t * mod, const std::string & match, bool full_search);

		static void getCurrentStack(int thread_id, void* rbp, void* rsp, void* rip);
	};
} // namespace drace