#include "drace-client.h"
#include "symbols.h"
#include "module-tracker.h"

#include <string>
#include <sstream>

std::string Symbols::get_bb_symbol(app_pc pc) {
	auto modc = module_tracker->get_module_containing(pc);
	
	if (modc.first) {
		// Reverse search from pc until symbol can be decoded
		auto offset = pc - modc.second->base;
		for (; offset >= 0; --offset) {
			drsym_error_t err = drsym_lookup_address(modc.second->info->full_path, offset, &syminfo, DRSYM_DEMANGLE);
			if (err == DRSYM_SUCCESS || err == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
				return std::string(syminfo.name);
			}
		}
	}
	return std::string("unknown");
}

SymbolLocation Symbols::get_symbol_info(app_pc pc) {
	SymbolLocation sloc;
	sloc.pc = pc;

	auto modc = module_tracker->get_module_containing(pc);
	if (modc.first) {

			sloc.mod_base = modc.second->base;
			sloc.mod_end = modc.second->end;
			sloc.mod_name = dr_module_preferred_name(modc.second->info);

			// Reverse search from pc until symbol can be decoded
			int offset = pc - modc.second->base;
			for (; offset >= 0; --offset) {
				drsym_error_t err = drsym_lookup_address(modc.second->info->full_path, offset, &syminfo, DRSYM_DEMANGLE);
				if (err == DRSYM_SUCCESS || err == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
					sloc.sym_name = syminfo.name;
					if (err != DRSYM_ERROR_LINE_NOT_AVAILABLE) {
						sloc.file = syminfo.file;
						sloc.line = syminfo.line;
						sloc.line_offs = syminfo.line_offs;
					}
					break;
				}
			}
	}
	return sloc;
}

void Symbols::print_bb_symbols(void) {
	drsym_error_t err;
	dr_printf("\n===\nModules:\n");
	for (const auto & m : module_tracker->modules) {
		syminfo.start_offs = 0;
		syminfo.end_offs = 0;
		dr_printf("\t- [%c] %s [%s]\n", (m.loaded ? '+' : '-'), dr_module_preferred_name(m.info), m.info->full_path);
		int modsize = m.end - m.base;
		for (int j = 0; j < modsize; j++) {
			int old = (syminfo.start_offs <= j && syminfo.end_offs >= j);
			if (!old)
				err = drsym_lookup_address(m.info->full_path, j, &syminfo, DRSYM_DEMANGLE);
			if (old || err == DRSYM_SUCCESS || err == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
				dr_printf("\t\t- basic_block " PFX " - [%08x -- %08x] %s\n", m.base + j, syminfo.start_offs, syminfo.end_offs, syminfo.name);
			}
			else {
				dr_printf("\t\t- basic_block " PFX "\n", m.base + j);
			}
		}
	}
}

void Symbols::create_drsym_info() {
	syminfo.struct_size = sizeof(drsym_info_t);
	syminfo.debug_kind = DRSYM_SYMBOLS;
	syminfo.name_size = buffer_size;
	syminfo.file_size = buffer_size;
	syminfo.file = new char[buffer_size];
	syminfo.name = new char[buffer_size];
}

void Symbols::free_drsmy_info() {
	delete syminfo.file;
	delete syminfo.name;
}
