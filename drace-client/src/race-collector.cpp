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

#include "race-collector.h"
#include "globals.h"
#include "memory-tracker.h"
#include "race-filter.h"
#include "statistics.h"

// clang-format off
#include <dr_api.h>
#include <drmgr.h>
// clang-format on

namespace drace {

RaceCollector* RaceCollector::_instance;

void RaceCollector::add_race(const Detector::Race* r) {
  if (num_races() > MAX) return;

#ifdef WINDOWS
  auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(
      clock_t::now() - _start_time);
#else
  // \todo when running under DR, calling clock::now()
  //       leads to a segfault
  auto ttr = std::chrono::milliseconds(0);
#endif

  std::lock_guard<DrLock> lock(_races_lock);

  if (!filter_duplicates(r) && !filter_excluded(r)) {
    DR_ASSERT(r->first.stack_size > 0);
    DR_ASSERT(r->second.stack_size > 0);

    _races.emplace_back(*r, ttr);
    if (!_delayed_lookup) {
      resolve_race(_races.back());
      if (_filter->check_suppress(_races.back())) {
        return;
      }
    }
    forward_race(_races.back());
    // destroy race in streaming mode
    if (!_delayed_lookup) {
      _races.pop_back();
    }
    _race_count += 1;
  }
}

void RaceCollector::resolve_all() {
  std::lock_guard<DrLock> lock(_races_lock);

  for (auto r = _races.begin(); r != _races.end();) {
    resolve_race(*r);
    if (_filter->check_suppress(*r)) {
      _race_count -= 1;
      r = _races.erase(
          r);  // erase returns iterator to element after the erased one
    } else {
      r++;
    }
  }
}

void RaceCollector::race_collector_add_race(const Detector::Race* r) {
  RaceCollector::_instance->add_race(r);
  // for benchmarking and testing
  if (params.break_on_race) {
    ShadowThreadState& data = thread_state.getSlot();
    data.stats.print_summary(log_target);
    dr_flush_file(log_target);
    dr_abort();
  }
}

bool RaceCollector::filter_excluded(const Detector::Race* r) {
  DR_ASSERT(r != nullptr);
  if (r->first.stack_size == 0 || r->second.stack_size == 0) return false;

  // PC is null
  if (r->first.stack_trace[r->first.stack_size - 1] == 0x0) return true;
  if (r->second.stack_trace[r->second.stack_size - 1] == 0x0) return true;

  return false;
}

bool RaceCollector::filter_duplicates(const Detector::Race* r) {
  // TODO: add more precise control over suppressions
  if (params.suppression_level == 0) return false;

  uintptr_t hash = r->first.stack_trace[0] ^ (r->second.stack_trace[0] << 1);
  if (_racy_stacks.count(hash) == 0) {
    // suppress this race
    _racy_stacks.insert(hash);
    return false;
  }
  return true;
}

void RaceCollector::resolve_symbols(race::ResolvedAccess& ra) {
  for (unsigned i = 0; i < ra.stack_size; ++i) {
    ra.resolved_stack.emplace_back(
        _syms->get_symbol_info((app_pc)ra.stack_trace[i]));
  }

#if 0
            // TODO: possibly use this information to refine callstack
            void* drcontext = dr_get_current_drcontext();
            dr_mcontext_t mc;
            mc.size = sizeof(dr_mcontext_t);
            mc.flags = DR_MC_ALL;
            dr_get_mcontext(drcontext, &mc);
#endif
}

}  // namespace drace
