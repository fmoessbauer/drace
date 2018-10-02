#include "globals.h"
#include "symbols.h"
#include "Module.h"
#include "MSR.h"
#include "ipc/SyncSHMDriver.h"

#include <string>
#include <sstream>
#include <algorithm>

std::string Symbols::get_bb_symbol(app_pc pc) {
	auto modptr = module_tracker->get_module_containing(pc);
	
	if (modptr) {
		// Reverse search from pc until symbol can be decoded
		uint64_t offset = pc - modptr->base;
		auto limit = std::max((uint64_t)0, offset - (uint64_t)max_distance);
		for (; offset >= limit; --offset) {
			drsym_error_t err = drsym_lookup_address(modptr->info->full_path, offset, &syminfo, DRSYM_DEMANGLE);
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

	auto modptr = module_tracker->get_module_containing(pc);
	// Not (Jitted PC or PC is in managed module)
	// OR managed module, but MSR is not attached
	if (modptr && ((modptr->modtype == module::Metadata::MOD_TYPE_FLAGS::NATIVE) 
		|| (modptr->modtype == module::Metadata::MOD_TYPE_FLAGS::MANAGED && !shmdriver))) {

			sloc.mod_base = modptr->base;
			sloc.mod_end  = modptr->end;
			sloc.mod_name = dr_module_preferred_name(modptr->info);

			// Reverse search from pc until symbol can be decoded
			int offset = pc - modptr->base;
			auto limit = std::max((uint64_t)0, offset - (uint64_t)max_distance);
			for (; offset >= limit; --offset) {
				drsym_error_t err = drsym_lookup_address(modptr->info->full_path, offset, &syminfo, DRSYM_DEMANGLE);
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
	else {
		// Managed Code
		if (shmdriver) {
			const auto & sym = MSR::lookup_address(pc);
			sloc.mod_name = sym.module.data();
			sloc.sym_name = sym.function.data();
			sloc.file = sym.path.data();
			// if the PC is not JITTED, try to get native module
			if (sloc.mod_name.empty() && modptr) {
				sloc.mod_name = dr_module_preferred_name(modptr->info);
				sloc.mod_base = modptr->base;
				sloc.mod_end  = modptr->end;
			}
			// we should never get here, as this must be jitted code
			// where we have symbol information, but no module
			if (sloc.mod_name.empty() && !sloc.sym_name.empty()) {
				sloc.mod_name = "JIT";
			}
		}
	}
	return sloc;
}

bool Symbols::debug_info_available(const module_data_t *mod) const {
	drsym_debug_kind_t flags;
	drsym_error_t error;
	
	error = drsym_get_module_debug_kind(mod->full_path, &flags);
	
	if (error == DRSYM_SUCCESS) {
		if (flags & DRSYM_SYMBOLS) return true;
	}
	return false;
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
	delete[] syminfo.file;
	delete[] syminfo.name;
}
