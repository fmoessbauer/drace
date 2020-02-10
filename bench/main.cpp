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

#include <benchmark/benchmark.h>

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);

  benchmark::RunSpecifiedBenchmarks();
}
