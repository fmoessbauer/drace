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
#include <detector/detector_if.h>

#include <iostream>

// TSAN can only be initialized once (BUG)
#if 0
static void DetectorRW(benchmark::State& state) {
	int num_threads = 8;

	detector::init(0, nullptr, nullptr);

	for (auto _ : state) {
		state.PauseTiming();
		std::vector<detector::tls_t> tls(num_threads);
		// generate threads
		for (int i = 0; i < num_threads; ++i) {
			detector::fork(1, i+2, &(tls[i]));
		}
		state.ResumeTiming();

		for (int i = 0; i < 10000; ++i) {
			int tid = i % num_threads;
			detector::write(tls[tid], (void*)i, (void*)i, 1);
		}

		// cleanup threads
		for (int i = 1; i < num_threads; ++i) {
			detector::join(1, i+1);
		}
	}
	detector::finalize();
}
// Register the function as a benchmark
BENCHMARK(DetectorRW);
#endif
