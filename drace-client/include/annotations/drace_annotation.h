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

/**
 * This header provides macros for annotating synchronisation events for DRace.
 * To enable annotations, set the define DRACE_ANNOTATION before including this
 * header. If this define is not set, the macros evaluate to a nop
 *
 * The annotation functions (prefix __drace_) are exported symbols
 * so that they can be found even if no debug information is available
 *
 * The following macros are available:
 *
 * \c DRACE_HAPPENS_BEFORE(identifier)
 * \c DRACE_HAPPENS_AFTER(identifier)
 *
 * Exclude a code region
 * \cDRACE_ENTER_EXCLUDE()
 * \cDRACE_LEAVE_EXCLUDE()
 */

#ifdef DRACE_ANNOTATION

#ifdef __unix__
#define DRACE_ANNOTATION_EXPORT
#elif defined(_WIN32) || defined(WIN32)
#define DRACE_ANNOTATION_EXPORT __declspec(dllexport)
#endif

extern "C" {

#pragma optimize("", off)
DRACE_ANNOTATION_EXPORT void __drace_happens_before(void* identifier) {
  // cppcheck-suppress unreadVariable
  volatile void* noopt = identifier;
}
#pragma optimize("", on)

#pragma optimize("", off)
DRACE_ANNOTATION_EXPORT void __drace_happens_after(void* identifier) {
  // cppcheck-suppress unreadVariable
  volatile void* noopt = identifier;
}
#pragma optimize("", on)

#pragma optimize("", off)
DRACE_ANNOTATION_EXPORT void __drace_enter_exclude() {
  int var;
  // cppcheck-suppress unreadVariable
  volatile void* noopt = &var;
}
#pragma optimize("", on)

#pragma optimize("", off)
DRACE_ANNOTATION_EXPORT void __drace_leave_exclude() {
  int var;
  // cppcheck-suppress unreadVariable
  volatile void* noopt = &var;
}
#pragma optimize("", on)
}

#define DRACE_HAPPENS_BEFORE(identifier)       \
  do {                                         \
    __drace_happens_before((void*)identifier); \
  } while (0)
#define DRACE_HAPPENS_AFTER(identifier)       \
  do {                                        \
    __drace_happens_after((void*)identifier); \
  } while (0)

#define DRACE_ENTER_EXCLUDE() \
  do {                        \
    __drace_enter_exclude();  \
  } while (0)
#define DRACE_LEAVE_EXCLUDE() \
  do {                        \
    __drace_leave_exclude();  \
  } while (0)
#else
#define DRACE_HAPPENS_BEFORE(identifier)
#define DRACE_HAPPENS_AFTER(identifier)
#define DRACE_ENTER_EXCLUDE()
#define DRACE_LEAVE_EXCLUDE()
#endif
