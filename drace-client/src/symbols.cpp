#include "drace-client.h"
#include "symbols.h"
#include "module-tracker.h"

#include <string>
#include <sstream>

using module_tracker::module_info_t;

/* Initialize symbol lookup helpers */
void symbols::init() {
	drsym_init(0);
	syminfo = create_drsym_info();
}

/* Cleanup symbol lookup helpers */
void symbols::finalize() {
	free_drsmy_info(syminfo);
	drsym_exit();
}
/* Create global symbol lookup data structures */
static drsym_info_t* symbols::create_drsym_info() {
	drsym_info_t* info;
	info = (drsym_info_t*)malloc(sizeof(drsym_info_t));
	info->struct_size = sizeof(drsym_info_t);
	info->debug_kind = DRSYM_SYMBOLS;
	info->name_size = 256;
	info->file_size = 256;
	info->file = (char*)malloc(256);
	info->name = (char*)malloc(256);
	return info;
}

/* Cleanup global symbol lookup data structures */
static void symbols::free_drsmy_info(drsym_info_t * info) {
	if (info->file != NULL)
		free(info->file);
	if (info->name != NULL)
		free(info->name);
	free(info);
}

/* Get last known symbol near the given location
*  Internally a reverse-search is performed starting at the given pc.
*  When the first symbol lookup was successful, the search is stopped.
*
* \WARNING If no debug information is available the returned symbol might
		   be misleading, as the search stops at the first imported or exported
		   function.
*/
std::string symbols::get_bb_symbol(app_pc pc) {
	module_info_t current(pc, nullptr);
	auto m_it = modules.lower_bound(current);

	if (m_it != modules.end() && pc < m_it->end) {
		// Reverse search from pc until symbol can be decoded
		int offset = pc - m_it->base;
		for (; offset >= 0; --offset) {
			drsym_error_t err = drsym_lookup_address(m_it->info->full_path, offset, syminfo, DRSYM_DEMANGLE);
			if (err == DRSYM_SUCCESS || err == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
				return std::string(syminfo->name);
			}
		}
	}
	return std::string("unknown");
}

/* Get last known symbol including as much information as possible.
*  Internally a reverse-search is performed starting at the given pc.
*  When the first symbol lookup was successful, the search is stopped.
*
* \WARNING If no debug information is available the returned symbol might
be misleading, as the search stops at the first imported or exported
function.
*/
SymbolLocation symbols::get_symbol_info(app_pc pc) {
	SymbolLocation sloc;
	sloc.pc = pc;

	module_info_t current(pc, nullptr);
	auto m_it = modules.lower_bound(current);
	if (m_it != modules.end() && pc < m_it->end) {

		if (m_it != modules.end() && pc < m_it->end) {

			sloc.mod_base = m_it->base;
			sloc.mod_end = m_it->end;
			sloc.mod_name = dr_module_preferred_name(m_it->info);

			// Reverse search from pc until symbol can be decoded
			int offset = pc - m_it->base;
			for (; offset >= 0; --offset) {
				drsym_error_t err = drsym_lookup_address(m_it->info->full_path, offset, syminfo, DRSYM_DEMANGLE);
				if (err == DRSYM_SUCCESS || err == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
					sloc.sym_name = syminfo->name;
					if (err != DRSYM_ERROR_LINE_NOT_AVAILABLE) {
						sloc.file = syminfo->file;
						sloc.line = syminfo->line;
						sloc.line_offs = syminfo->line_offs;
					}
					break;
				}
			}
		}
	}
	return sloc;
}

/* Print the related symbol information for each basic block */
void symbols::print_bb_symbols(void) {
	int i, j;
	drsym_error_t err;
	dr_printf("\n===\nModules:\n");
	for (const auto & m : modules) {
		syminfo->start_offs = 0;
		syminfo->end_offs = 0;
		dr_printf("\t- [%c] %s [%s]\n", (m.loaded ? '+' : '-'), dr_module_preferred_name(m.info), m.info->full_path);
		int modsize = m.end - m.base;
		for (j = 0; j < modsize; j++) {
			int old = (syminfo->start_offs <= j && syminfo->end_offs >= j);
			if (!old)
				err = drsym_lookup_address(m.info->full_path, j, syminfo, DRSYM_DEMANGLE);
			if (old || err == DRSYM_SUCCESS || err == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
				dr_printf("\t\t- basic_block " PFX " - [%08x -- %08x] %s\n", m.base + j, syminfo->start_offs, syminfo->end_offs, syminfo->name);
			}
			else {
				dr_printf("\t\t- basic_block " PFX "\n", m.base + j);
			}
		}
	}
}
