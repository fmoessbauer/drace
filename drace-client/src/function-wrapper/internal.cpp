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
#include <drwrap.h>
#include <algorithm>
#include <cstdint>

#include <detector/Detector.h>
#include "function-wrapper.h"
#include "globals.h"
#include "memory-tracker.h"

namespace drace {
namespace funwrap {
bool internal::wrap_function_clbck(const char *name, size_t modoffs,
                                   void *data) {
  wrap_info_t *info = (wrap_info_t *)data;
  bool ok = drwrap_wrap_ex(info->mod->start + modoffs, info->pre, info->post,
                           (void *)name, DRWRAP_CALLCONV_FASTCALL);
  if (ok)
    LOG_INFO(0, "wrapped function %s at %p", name, info->mod->start + modoffs);

  // Exact matches only, hence quit after each symbol
  return false;
}
}  // namespace funwrap
}  // namespace drace
