#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <dr_api.h>
#include <drsyms.h>

#include <string>
#include <sstream>

namespace drace {

    /// Information related to a symbol
	class SymbolLocation {
	public:
		app_pc          pc{ 0 };
		app_pc          mod_base{ nullptr };
		app_pc          mod_end{ nullptr };
		std::string     mod_name;
		std::string     sym_name;
		std::string     file;
		uint64          line{ 0 };
		size_t          line_offs{ 0 };

		/** Pretty print symbol location */
		std::string get_pretty() const {
            // we use c-style formatting here, as we cannot
            // use std::stringstream (see i#9)
            constexpr int bufsize = 256;
            char * strbuf = (char*)dr_thread_alloc(dr_get_current_drcontext(), bufsize); //strbufptr.get();

            dr_snprintf(strbuf, bufsize, "PC %p ", pc);
			if (nullptr != mod_base) {
                size_t len = strlen(strbuf);
                dr_snprintf(strbuf + len, bufsize - len,
                    "(rel: %#010x)\n", (void*)(pc - mod_base));
			}
			else {
                size_t len = strlen(strbuf);
                dr_snprintf(strbuf + len, bufsize - len,
                    "(dynamic code)\n");
			}
			if (mod_name != "") {
                size_t len = strlen(strbuf);
                dr_snprintf(strbuf + len, bufsize - len,
                    "\tModule %s\n", mod_name.c_str());

				if (sym_name != "") {
                    // we overwrite the last \n here
                    size_t len = strlen(strbuf);
                    dr_snprintf(strbuf + len-1, bufsize - len,
                        " - %s\n", sym_name.c_str());
                }

				if (file != "") {
                    size_t len = strlen(strbuf);
                    dr_snprintf(strbuf + len, bufsize - len,
                        "\tFile %s:%i + %i\n", file.c_str(), (int)line, (int)line_offs);
				}
			}
            auto str = std::string(strbuf);
            dr_thread_free(dr_get_current_drcontext(), strbuf, bufsize);
            return str;
		}
	};

	/// Symbol Access Lib Functions
	class Symbols {
		/* Maximum distance between a pc and the first found symbol */
		static constexpr std::ptrdiff_t max_distance = 32;
		/* Maximum length of file pathes and sym names */
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

		/** Get last known symbol near the given location
		*  Internally a reverse-search is performed starting at the given pc.
		*  When the first symbol lookup was successful, the search is stopped.
		*
		* \warning If no debug information is available the returned symbol might
		*		   be misleading, as the search stops at the first imported or exported
		*		   function.
		*/
		std::string get_bb_symbol(app_pc pc);

		/** Get last known symbol including as much information as possible.
		*  Internally a reverse-search is performed starting at the given pc.
		*  When the first symbol lookup was successful, the search is stopped.
		*
		* \warning If no debug information is available the returned symbol might
		*          be misleading, as the search stops at the first imported or exported
		*          function.
		*/
		SymbolLocation get_symbol_info(app_pc pc);

		/** Returns true if debug info is available for this module
		* Returns false if only exports are available
		*/
		bool debug_info_available(const module_data_t *mod) const;

	private:
		/** Create global symbol lookup data structures */
		void create_drsym_info();

		/** Cleanup global symbol lookup data structures */
		void free_drsmy_info();
	};
} // namespace drace
