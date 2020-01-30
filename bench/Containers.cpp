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

#include <unordered_map>
#include <random>
#include <limits>

/* This benchmark simulates the load of mutex tracking in the drace client
*  Various numbers of concurrent mutexes are simulated, showing
*  that a linear search is much faster (150ns vs. 750ns) for this scenario
*/

std::mt19937 prng(42);
std::uniform_real_distribution<float> dropout(0, 1);

template<typename dist>
inline uint64_t generate_key(dist & d) {
	return d(prng) * (std::numeric_limits<uint32_t>::max() / d.max()); // Keys shoud approx memory locations
}

static void StdUMapMutexLoad(benchmark::State& state) {
	std::unordered_map<uint64_t, int> map;
	std::uniform_int_distribution<uint64_t> dist(0, state.range(0));

	for (auto _ : state) {
		auto key = generate_key(dist);
		map[key]++;

		if (dropout(prng) > 0.6) {
			map.erase(key);
		}
	}
	state.counters["size"] = static_cast<double>(map.size());
}

static void LinearSearchMutexLoad(benchmark::State& state) {
	uint64_t * buf = new uint64_t[state.range(0)*2];
	std::uniform_int_distribution<uint64_t> dist(0, state.range(0));

	size_t size = 0;
	for (auto _ : state) {
        size_t i;
		auto key = generate_key(dist);
		// increment key or add
		for (i = 0; i < size; i+=2) {
			if (buf[i] == key) {
				buf[i + 1]++;
				break;
			}
		}
		if (i == size) {
			buf[size] = key;
			buf[size + 1] = 1;
			size += 2;
		}

		if (dropout(prng) > 0.6) {
			for (i = 0; i < size; i += 2) {
				if (buf[i] == key) {
					if (size != 2) {
						buf[i] = buf[size - 2];
						buf[i + 1] = buf[size - 1];
					}
					size -= 2;
					break;
				}
			}
		}
	}
	state.counters["size"] = static_cast<double>(size/2);

	delete[] buf;
}

// Register the function as a benchmark
BENCHMARK(StdUMapMutexLoad)->RangeMultiplier(2)->Range(1, 1024);
BENCHMARK(LinearSearchMutexLoad)->RangeMultiplier(2)->Range(1, 1024);
