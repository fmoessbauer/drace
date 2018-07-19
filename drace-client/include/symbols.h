#pragma once
#include <dr_api.h>
#include <drsyms.h>

#include <string>
#include <sstream>

class SymbolLocation {
public:
	app_pc          pc{ 0 };
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
				<< " to " << (void*)mod_end;
			if (file != "") {
				result << "\n\tFile " << file << ":" << std::dec
					<< line << " + " << line_offs;
			}
		}
		return result.str();
	}
};

/* Symbol Access Lib Functions */
class Symbols {
	static constexpr int buffer_size = 256;
	drsym_info_t syminfo;

public:
	Symbols() {
		drsym_init(0);
		create_drsym_info();
	}

	~Symbols() {
		free_drsmy_info();
		drsym_exit();
	}

	/* Get last known symbol near the given location
	*  Internally a reverse-search is performed starting at the given pc.
	*  When the first symbol lookup was successful, the search is stopped.
	*
	* \WARNING If no debug information is available the returned symbol might
	*		   be misleading, as the search stops at the first imported or exported
	*		   function.
	*/
	std::string get_bb_symbol(app_pc pc);

	/* Get last known symbol including as much information as possible.
	*  Internally a reverse-search is performed starting at the given pc.
	*  When the first symbol lookup was successful, the search is stopped.
	*
	* \WARNING If no debug information is available the returned symbol might
	*          be misleading, as the search stops at the first imported or exported
	*          function.
	*/
	SymbolLocation get_symbol_info(app_pc pc);

	/* Print the related symbol information for each basic block */
	void print_bb_symbols();

private:
	/* Create global symbol lookup data structures */
	void create_drsym_info();

	/* Cleanup global symbol lookup data structures */
	void free_drsmy_info();
};
