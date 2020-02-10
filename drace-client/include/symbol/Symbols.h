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

#include <sstream>
#include <string>

#ifndef TESTING
#include <dr_api.h>
#include <drsyms.h>
#endif

#include "symbol/SymbolLocation.h"

#ifdef TESTING
class drsym_info_t {};
class module_data_t;
#endif

namespace drace {
namespace symbol {

/// Symbol Access Lib Functions
class Symbols {
  /* Maximum distance between a pc and the first found symbol */
  static constexpr std::ptrdiff_t max_distance = 32;
  /* Maximum length of file pathes and sym names */
  static constexpr int buffer_size = 256;
  drsym_info_t syminfo{};

 public:
  explicit Symbols();
  ~Symbols();

  /** Get last known symbol near the given location
   *  Internally a reverse-search is performed starting at the given pc.
   *  When the first symbol lookup was successful, the search is stopped.
   *
   * \warning If no debug information is available the returned symbol might
   *		   be misleading, as the search stops at the first imported or
   *exported function.
   */
  std::string get_bb_symbol(app_pc pc);

  /** Get last known symbol including as much information as possible.
   *  Internally a reverse-search is performed starting at the given pc.
   *  When the first symbol lookup was successful, the search is stopped.
   *
   * \warning If no debug information is available the returned symbol might
   *          be misleading, as the search stops at the first imported or
   * exported function.
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

}  // namespace symbol
}  // namespace drace
