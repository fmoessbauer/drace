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
#include <detector/Detector.h>

#include "ipc/DrLock.h"
#include "race/DecoratedRace.h"
#include "sink/sink.h"

#include <chrono>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace drace {

// forward-decls
class RaceFilter;

/**
 * \brief collects all detected races and manages symbol resolving
 *
 * Note for developers: Be very cautious with concurrency in this class.
 * Locking has to be avoided if possible, as this might interfere with
 * locks held by the application.
 */
class RaceCollector {
 public:
  /** Maximum number of races to collect in delayed mode */
  static constexpr int MAX = 200;

 private:
  static RaceCollector* _instance;

 private:
  using RaceCollectionT = std::vector<race::DecoratedRace>;
  using entry_t = race::DecoratedRace;
  using clock_t = std::chrono::high_resolution_clock;
  using tp_t = decltype(clock_t::now());

  RaceCollectionT _races;
  /// guards all accesses to the _races container
  DrLock _races_lock;
  unsigned long _race_count{0};

  bool _delayed_lookup{false};
  std::shared_ptr<symbol::Symbols> _syms;
  tp_t _start_time;
  std::set<uintptr_t> _racy_stacks;

  std::vector<std::shared_ptr<sink::Sink>> _sinks;

  std::shared_ptr<RaceFilter> _filter;

 public:
  RaceCollector(bool delayed_lookup,
                const std::shared_ptr<symbol::Symbols>& symbols,
                std::shared_ptr<RaceFilter> filter)
      : _delayed_lookup(delayed_lookup),
        _syms(symbols),
        _start_time(clock_t::now()),
        _filter(filter)

  {
    _races.reserve(1);
    _instance = this;
  }

  ~RaceCollector() {}

  /**
   * register a sink that is notified on each race
   * \note not-threadsafe
   */
  inline void register_sink(const std::shared_ptr<sink::Sink>& sink) {
    _sinks.push_back(sink);
  }

  /** Adds a race and updates histogram
   *
   * \note threadsafe
   */
  void add_race(const Detector::Race* r);

  /**
   * Resolves all unresolved race entries
   *
   * \note threadsafe
   */
  void resolve_all();

  /**
   * In delayed mode, return data-races.
   * Otherwise return empty container
   *
   * \note not-threadsafe
   */
  inline const std::vector<race::DecoratedRace>& get_races() const {
    return _races;
  }

  /**
   * return the number of observed races
   */
  inline unsigned long num_races() const { return _race_count; }

  /**
   * This function provides a callback to the RaceCollector::add_race
   * on the singleton object. As we have to pass this callback to
   * as a function pointer to c, we cannot use std::bind
   */
  static void race_collector_add_race(const Detector::Race* r);

 private:
  /**
   * Filter false-positive data-races
   * \return true if race is suppressed
   */
  bool filter_excluded(const Detector::Race* r);

  /**
   * suppress this race if similar race is already reported
   * \return true if race is suppressed
   * \note   not-threadsafe
   */
  bool filter_duplicates(const Detector::Race* r);

  /** Takes a detector Access Entry, resolves symbols and converts it to a
   * ResolvedAccess */
  void resolve_symbols(race::ResolvedAccess& ra);

  /** resolve a single race */
  inline void resolve_race(race::DecoratedRace& race) {
    if (!race.is_resolved) {
      resolve_symbols(race.first);
      resolve_symbols(race.second);
    }
    race.is_resolved = true;
  }

  /**
   * forward a single race to the sinks
   *
   * \note not-threadsafe
   */
  inline void forward_race(const race::DecoratedRace& race) const {
    for (const auto& s : _sinks) {
      s->process_single_race(race);
    }
  }
};

}  // namespace drace
