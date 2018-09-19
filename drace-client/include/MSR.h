#pragma once

#include <dr_api.h>

/* Provides routines to perform communication with MSR */
class MSR {
public:
	static bool attach(const module_data_t * mod);
	static bool request_symbols(const module_data_t * mod);
};
