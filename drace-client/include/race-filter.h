
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

#ifndef RACEFILTER_H
#define RACEFILTER_H

#include <detector/Detector.h>
#include <race/DecoratedRace.h>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace drace {

class RaceFilter {
  std::vector<std::string> rtos_list;  // race top of stack
  std::vector<std::string> race_list;
  void normalize_string(std::string &expr);

  bool check_race(const drace::race::DecoratedRace &race);
  bool check_rtos(const drace::race::DecoratedRace &race);
  void init(std::istream &content);

 public:
  explicit RaceFilter(std::istream &content);
  explicit RaceFilter(const std::string &filename,
                      const std::string &hint = {});

  bool check_suppress(const drace::race::DecoratedRace &race);
  void print_list();
};

}  // namespace drace

#endif  // RACEFILTER_H
