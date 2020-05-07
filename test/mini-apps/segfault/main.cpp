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

#include <windows.h>
#include <sstream>

LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
  printf("Fatal: Unhandled exception %#08x\n",
         pExceptionInfo->ExceptionRecord->ExceptionCode);
  exit(1);
  return EXCEPTION_CONTINUE_SEARCH;
}

int main() {
  SetUnhandledExceptionFilter(TopLevelExceptionHandler);
  int *v = 0;
  v[12] = 0;  // trigger the fault
  return 0;
}
