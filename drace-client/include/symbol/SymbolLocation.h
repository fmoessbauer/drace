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

#include <cstdint>
#include <string>

#ifndef TESTING
#include <dr_api.h>
#else
typedef char* app_pc;
#endif

namespace drace {
namespace symbol {
/// Information related to a symbol
class SymbolLocation {
 public:
  app_pc pc{0};
  app_pc mod_base{nullptr};
  app_pc mod_end{nullptr};
  std::string mod_name;
  std::string sym_name;
  std::string file;
  uintptr_t line{0};
  size_t line_offs{0};

  /** Pretty print symbol location */
  std::string get_pretty() const;
};
}  // namespace symbol
}  // namespace drace
