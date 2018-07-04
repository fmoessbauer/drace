#include "drace-client.h"
#include "symbols.h"
#include "module-tracker.h"

#include <string>
#include <sstream>

void symbols::init() {
	drsym_init(0);
	syminfo = create_drsym_info();
}

void symbols::finalize() {
	free_drsmy_info(syminfo);
	drsym_exit();
}

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

static void symbols::free_drsmy_info(drsym_info_t * info) {
	if (info->file != NULL)
		free(info->file);
	if (info->name != NULL)
		free(info->name);
	free(info);
}

// TODO: Optimize for cases where module is already known
std::string symbols::get_bb_symbol(app_pc pc) {
	// TODO: get module
	module_tracker::module_info_t current(pc, nullptr);
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

std::string symbols::get_symbol_info(app_pc pc) {
	module_tracker::module_info_t current(pc, nullptr);
	std::stringstream result;
	auto m_it = modules.lower_bound(current);

	result << "PC " << std::hex << (void*)pc << "\n";

	if (m_it != modules.end() && pc < m_it->end) {
		const char * name = dr_module_preferred_name(m_it->info);

		// Reverse search from pc until symbol can be decoded
		int offset = pc - m_it->base;
		for (; offset >= 0; --offset) {
			drsym_error_t err = drsym_lookup_address(m_it->info->full_path, offset, syminfo, DRSYM_DEMANGLE);
			if (err == DRSYM_SUCCESS || err == DRSYM_ERROR_LINE_NOT_AVAILABLE) {
				result << "\tModule " << name << " - " << syminfo->name << "\n"
					   << "\tfrom " << (void*)m_it->base
					   << " to "    << (void*)m_it->end;
				if (err != DRSYM_ERROR_LINE_NOT_AVAILABLE) {
					result << "\n\tFile " << syminfo->file << ":" << std::dec
						<< syminfo->line << " + " << syminfo->line_offs;
				}
				break;
			}
		}
	}
	return result.str();
}

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
