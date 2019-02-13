/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */

#include <thread>
#include <mutex>

/**
\brief This code serves as a test for mutex detection in the drace client
*/

static std::mutex mx;

void inc(int * v) {
	for (int i = 0; i < 1000; ++i) {
		mx.lock();
		int val = *v;
		++val;
		std::this_thread::yield();
		*v = val;
		mx.unlock();
	}
}

int main() {
	int var = 0;

	auto ta = std::thread(&inc, &var);
	auto tb = std::thread(&inc, &var);

	ta.join();
	tb.join();

	return var;
}
