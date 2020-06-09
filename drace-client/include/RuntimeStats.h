#pragma once
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

#include "statistics.h"
#include "util.h"

#include <dr_api.h>

namespace drace {

/**
 * \brief Orchestrates the global statistics handling
 *
 * This singleton component is responsible for registering process-wide
 * events regarding runtime information.
 */
class RuntimeStats : public Statistics {
 public:
  RuntimeStats() : Statistics(0) {
    if (!drmgr_register_low_on_memory_event(event_low_on_memory)) {
      LOG_WARN(0, "Could not register low-on-memory event");
    }
  }
  ~RuntimeStats() { drmgr_unregister_low_on_memory_event(event_low_on_memory); }

 private:
  static void event_low_on_memory() { LOG_WARN(0, "DRace is low on memory"); }
};
}  // namespace drace
