#include <thread>
#include <atomic>
#include <array>
#include <vector>
#include <iostream>

// exclude races on IO without adding synchronisation
#define DRACE_ANNOTATION
#include "../../../drace-client/include/annotations/drace_annotation.h"

void increment_atomic(std::atomic<int> * x) {
	for (int i = 0; i < 1000; ++i) {
		x->fetch_add(1, std::memory_order_relaxed);
	}
}

void increment_mixed(std::atomic<int> * x) {
	// hacky cast to simulate misusage
	int * nonatomic = (int*)x;

	for (int i = 0; i < 10000; ++i) {
		if (i % 2 == 0) {
			x->fetch_add(1, std::memory_order_relaxed);
		}
		else {
			int tmp = *nonatomic;
			++tmp;
			std::this_thread::yield();
			*nonatomic = tmp;
		}
	}
}

/* if executed without argument, perform race-free atomic case.
*  Otherwise, also try racy mixed increment
*/
int main(int argc, char ** argv) {
	std::atomic<int> var1{ 0 };
	std::atomic<int> var2{ 0 };

	int threads_per_task = 2;
	std::vector<std::thread> threads;
	threads.reserve(2 * threads_per_task);

	for (int i = 0; i < threads_per_task; ++i) {
		threads.emplace_back(&increment_atomic, &var1);
		if (argc != 1) {
			threads.emplace_back(&increment_mixed, &var2);
		}
	}

	for (auto & th : threads) {
		th.join();
	}

	DRACE_ENTER_EXCLUDE();
	std::cout << "Inc Atomic x: " << var1.load(std::memory_order_relaxed) << std::endl;
	if (argc != 1) {
		std::cout << "Inc Mixed  x: " << var2.load(std::memory_order_relaxed) << std::endl;
	}
	DRACE_LEAVE_EXCLUDE();
}
