#include <thread>
#include <mutex>
#include <vector>
#include <iostream>

// This code serves as a test for mutex detection
// in the drace client

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

int main() {
	int var1 = 0;
	int var2 = 0;
	int threads_per_task = 10;
	std::vector<std::thread> threads;
	threads.reserve(2 * threads_per_task);

	for (int i = 0; i < threads_per_task; ++i) {
		threads.emplace_back(&lock_recusive, &var1, 3);
		threads.emplace_back(&try_lock, &var2);
	}

	for (auto & th : threads) {
		th.join();
	}
}
