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

#include "function-wrapper.h"

#include <string>
#include <vector>

namespace drace {
namespace funwrap {
// forward declare to avoid circular dependency
using wrapcb_pre_t = void(void *, void **);
using wrapcb_post_t = void(void *, void *);

namespace internal {
void wrap_dotnet_helper(uintptr_t addr);
bool wrap_function_clbck(const char *name, size_t modoffs, void *data);

bool mutex_wrap_callback(const char *name, size_t modoffs, void *data);
}  // namespace internal
}  // namespace funwrap
}  // namespace drace
