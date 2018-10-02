#pragma once

#include <dr_api.h>
#include "ipc/SMData.h"

/* Provides routines to perform communication with MSR */
class MSR {
public:
	static bool attach(const module_data_t * mod);
	static bool request_symbols(const module_data_t * mod);

	static ipc::SymbolInfo lookup_address(app_pc pc);
	static ipc::SymbolResponse search_symbol(const module_data_t * mod, const std::string & match);
};
