#include <thread>
#include <iostream>
#include <mutex>

#define NUM_INCREMENTS 1'000
#define USE_HEAP

//std::mutex mx;

inline void inc(int * v) {
	for (int i = 0; i < NUM_INCREMENTS; ++i) {
		int var = *v;
		++var;
		std::this_thread::yield();
		*v = var;
	}
}
void dec(int * v) {
	for (int i = 0; i < NUM_INCREMENTS; ++i) {
		int var = *v;
		--var;
		std::this_thread::yield();
		*v = var;
	}
}

int main() {
#ifdef USE_HEAP
	int * mem = new int[1];
	*mem = 0;
#else
	int var = 0;
	int * mem = &var;
#endif

	auto ta = std::thread(&inc, mem);
	auto tb = std::thread(&dec, mem);

	ta.join();
	tb.join();
	
	//mx.lock();
	std::cout << "EXPECTED: " << 0 << ", "
		<< "ACTUAL: " << *mem << ", "
		<< "LOCATION: " << std::hex << (uint64_t) (mem) << std::endl;
	//mx.unlock();
#ifdef USE_HEAP
	delete mem;
#endif
	return 0;
}
