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

#include <thread>
#include <mutex>
#include <array>
#include <vector>
#include <iostream>

#include <windows.h>

/**
\brief This code serves as a test for mutex detection in the drace client
*/

static std::mutex mx;
static std::recursive_mutex rmx;

void lock_recusive(int * v, int depth) {
	if (depth > 0) {
		std::lock_guard<decltype(rmx)> lg(rmx);
		++(*v);
		lock_recusive(v, --depth);
	}
	//std::cout << "Lock_recursive: bottom reached" << std::endl;
	return;
}

void try_lock(int * v) {
	const int steps  = 1000;
	unsigned  misses = 0;

	for (int i = 0; i < steps; ++i) {
		while (!mx.try_lock()) {
			++misses;
		}
		++(*v);
		mx.unlock();
	}
	//std::cout << "Try_lock: misses/round:" << (misses / steps) << std::endl;
}

void crit_sect(int * v, CRITICAL_SECTION * cs) {
	const int steps = 1000;
	
	for (int i = 0; i < steps; ++i) {
		EnterCriticalSection(cs);
		++(*v);
		LeaveCriticalSection(cs);
	}
	EnterCriticalSection(cs);
	// only the last value has to be accurate
	std::cout << "Value " << *v << " after " << steps << " rounds" << std::endl;
	LeaveCriticalSection(cs);
}

void local_buffer(int * v, CRITICAL_SECTION * cs) {
	int buffer[16];
	int * buffer2 = (int*)_alloca(16);
	buffer[*v] = *v;
	EnterCriticalSection(cs);
	std::cout << "Value " << buffer[*v] << " buffer @ " << buffer2 << std::endl;
	ULONG_PTR low, high;
	GetCurrentThreadStackLimits(&low, &high);
	std::cout << "Stack from " << (void*)low << " to " << (void*)high << std::endl;
	LeaveCriticalSection(cs);
}

int main() {
	std::array<int, 4> vars{0};
	CRITICAL_SECTION _cs;
	constexpr int threads_per_task = 10;
	std::vector<std::thread> threads;
	threads.reserve(4 * threads_per_task);

	InitializeCriticalSectionAndSpinCount(&_cs, 0);
	for (int i = 0; i < threads_per_task; ++i) {
		threads.emplace_back(&lock_recusive, &vars[0], 3);
		threads.emplace_back(&try_lock, &vars[1]);
		threads.emplace_back(&crit_sect, &vars[2], &_cs);
		threads.emplace_back(&local_buffer, &vars[3], &_cs);
	}

	for (auto & th : threads) {
		th.join();
	}
	DeleteCriticalSection(&_cs);
}
