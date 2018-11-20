#include <thread>
#include <array>
#include <vector>
#include <atomic>
#include <mutex>
#include <iostream>
#include <chrono>

// exclude races on IO without adding synchronisation,
// annotate spinlock with happens before
#define DRACE_ANNOTATION
#include "../../../drace-client/include/annotations/drace_annotation.h"
#include "../../../common/ipc/spinlock.h"

std::atomic<uint64_t> cntr{0};

/**
* Here, we have one access to the racy location per period.
* All other accesses are local-only.
*/
void no_sync(
	int tid,
	uint64_t * racy,
	unsigned period,
	uint64_t rounds,
	std::atomic<bool> * go)
{
	while (!go->load(std::memory_order_relaxed)) { }
	uint64_t tmp = 0;
	uint64_t boundary = period / (tid + 1) - 1;
	for (uint64_t i = 0; i < period * rounds; ++i) {
		if (i % period == boundary) {
			// racy write
			*racy = tmp;
			cntr.fetch_add(period, std::memory_order_relaxed);
		}
		else {
			++tmp;
		}
	}
}

/**
* Here, we have two accesses per period to the racy location:
* One is synchronized, one is racy.
* All other accesses are local only
*/
void sync(
	int tid,
	uint64_t * racy,
	unsigned period,
	uint64_t rounds,
	std::atomic<bool> * go)
{
	static ipc::spinlock mx;
	while (!go->load(std::memory_order_relaxed)) {}
	uint64_t tmp = 0;
	uint64_t boundary = period / (tid + 1) - 1;
	for (uint64_t i = 0; i < period * rounds; ++i) {
		if (i % period == tid) {
			// racy write
			*racy = tmp;
			cntr.fetch_add(period, std::memory_order_relaxed);
		}
		else if (i % period == boundary) {
			// for performance reasons, we exclude the mutex call itself
			DRACE_ENTER_EXCLUDE();
			mx.lock();
			DRACE_LEAVE_EXCLUDE();

			DRACE_HAPPENS_AFTER(&mx);
			*racy = tmp;
			DRACE_HAPPENS_BEFORE(&mx);

			DRACE_ENTER_EXCLUDE();
			mx.unlock();
			DRACE_LEAVE_EXCLUDE();
		}
		else {
			++tmp;
		}
	}
}

void watchdog(std::atomic<bool> * running) {
	using namespace std::chrono;

	DRACE_ENTER_EXCLUDE();
	uint64_t lastval = 0;
	uint64_t curval = 0;
	auto start    = system_clock::now();
	auto lasttime = start;
	decltype(lasttime) curtime;
	uint64_t sum_time_ms = 0;

	while (running->load(std::memory_order_relaxed)) {
		std::this_thread::sleep_for(seconds(1));
		curval = cntr.load(std::memory_order_relaxed);
		curtime = system_clock::now();
		auto valdiff = curval - lastval;
		auto timediff = duration_cast<milliseconds>(curtime - lasttime).count();
		auto ops_ms = valdiff / timediff;
		std::cout << "Perf: current " << ops_ms << " avg: " 
			<< curval / duration_cast<milliseconds>(curtime - start).count() 
			<< " ops/ms sum: " << curval << std::endl;
		lastval = curval;
		lasttime = curtime;
	}
	DRACE_LEAVE_EXCLUDE();
}

/** 
* Test tool to measure sampling based race detectors
* Param 1: Period of a race
* Param 2: n: 2^n repetitions
* <Param 3> 's' for synchronized test, empty for unsync test
*/
int main(int argc, char ** argv) {
	uint64_t racy = 0;
	std::atomic<bool> trigger{ false };

	int threads_per_task = 2;
	std::vector<std::thread> threads;
	threads.reserve(threads_per_task);

	unsigned period = std::atoi(argv[1]);
	unsigned rounds = std::atoi(argv[2]);
	bool mode_sync = (argc == 4) && (*argv[3] = 's');

	std::cout << "Period: " << period << std::endl;
	std::cout << "Rounds: 2^" << rounds << std::endl;
	if (mode_sync) {
		std::cout << "Mode: Sync"  << std::endl;
	}
	else {
		std::cout << "Mode: No-Sync" << std::endl;
	}

	std::atomic<bool> wd_running{ true };
	std::thread wd_thread(&watchdog, &wd_running);
	for (int i = 0; i < threads_per_task; ++i) {
		if (mode_sync) {
			threads.emplace_back(&sync, i, &racy, period, std::pow(2, rounds), &trigger);
		}
		else {
			threads.emplace_back(&no_sync, i, &racy, period, std::pow(2, rounds), &trigger);
		}
	}
	trigger.store(true, std::memory_order_relaxed);

	for (auto & th : threads) {
		th.join();
	}
	wd_running = false;

	DRACE_ENTER_EXCLUDE();
	std::cout << "Racy x: " << racy << std::endl;
	std::cout << "First Accesses: " << rounds << std::endl;
	DRACE_LEAVE_EXCLUDE();

	wd_thread.join();
}
