#pragma once
#include <dr_api.h>
#include <drsyms.h>

#include <string>

// Symbol Access Lib Functions
namespace symbols {
	static drsym_info_t* syminfo;

	void init();
	void finalize();

	static drsym_info_t* create_drsym_info(void);
	static void free_drsmy_info(drsym_info_t * info);

	std::string get_bb_symbol(app_pc pc);
	std::string get_symbol_info(app_pc pc);

	void print_bb_symbols(void);
}
