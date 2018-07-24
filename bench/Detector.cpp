#include "benchmark/benchmark.h"
#include <detector_if.h>

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
			detector::fork(0, i, tls[i]);
		}
		state.ResumeTiming();

		for (int i = 0; i < 10000; ++i) {
			int tid = i % num_threads + 1;
			detector::write(tid, (void*)i, (void*)i, 1, tls[tid]);
		}

		// cleanup threads
		for (int i = 0; i < num_threads; ++i) {
			detector::join(0, i, tls[i]);
		}
	}
	detector::finalize();
}
// Register the function as a benchmark
BENCHMARK(DetectorRW);

#endif