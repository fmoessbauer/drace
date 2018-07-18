#pragma once
#include <dr_api.h>
#include <drsyms.h>

#include <string>
#include <sstream>

// Symbol Access Lib Functions
namespace symbols {
	class symbol_location_t {
	public:
		app_pc          pc;
		app_pc          mod_base;
		app_pc          mod_end;
		std::string     mod_name;
		std::string     sym_name;
		std::string     file;
		uint64          line;
		size_t          line_offs;

		/* Pretty print symbol location */
		std::string get_pretty() const {
			std::stringstream result;
			result << "PC " << std::hex << (void*)pc << "\n";
			if (mod_name != "") {
				result << "\tModule " << mod_name;
				if (sym_name != "") {
					result << " - " << sym_name << "\n";
				}
				result << "\tfrom " << (void*)mod_base
					   << " to "    << (void*)mod_end;
				if (file != "") {
					result << "\n\tFile " << file << ":" << std::dec
						   << line << " + " << line_offs;
				}
			}
			return result.str();
		}
	};

	static drsym_info_t* syminfo;

	void init();
	void finalize();

	static drsym_info_t* create_drsym_info(void);
	static void free_drsmy_info(drsym_info_t * info);

	std::string get_bb_symbol(app_pc pc);
	symbol_location_t get_symbol_info(app_pc pc);

	void print_bb_symbols(void);
}
