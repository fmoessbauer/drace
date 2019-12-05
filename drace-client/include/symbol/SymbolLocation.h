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

#include <string>

namespace drace {
namespace symbol {
/// Information related to a symbol
	class SymbolLocation {
	public:
		byte*           pc{ 0 };
		byte*           mod_base{ nullptr };
		app_pc          mod_end{ nullptr };
		std::string     mod_name;
		std::string     sym_name;
		std::string     file;
		uint64          line{ 0 };
		size_t          line_offs{ 0 };

		/** Pretty print symbol location */
		std::string get_pretty() const;
	};
} // namespace symbol
} // namespace drace