
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2020 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RACEFILTER_H
#define RACEFILTER_H

#include <detector/Detector.h>
#include <race/DecoratedRace.h>
#include <chrono>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace drace {

/**
 * \brief Filter data-races based on various criteria
 */
class RaceFilter {
  std::vector<std::string> rtos_list;  // race top of stack
  std::vector<std::string> race_list;
  std::set<uint64_t> addr_list;
  void normalize_string(std::string &expr);

  bool check_race(const drace::race::DecoratedRace &race) const;
  bool check_rtos(const drace::race::DecoratedRace &race) const;
  void init(std::istream &content);

 public:
  RaceFilter() = default;

  /// constructor which takes a supression list as input stream
  explicit RaceFilter(std::istream &content);

  /// constructor which takes a filename and path
  RaceFilter(const std::string &filename, const std::string &hint = {});

  /// return true if race should be suppressed
  bool check_suppress(const drace::race::DecoratedRace &race) const;
  /// print a list of suppression criteria
  void print_list() const;

  /// add an address to the suppression list
  void suppress_addr(uint64_t addr);
};

}  // namespace drace

#endif  // RACEFILTER_H
