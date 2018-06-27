#include <thread>
#include <iostream>
#include <mutex>

#define NUM_INCREMENTS 1'000'00
#define USE_HEAP

std::mutex mx;

void inc(int * v) {
	//mx.lock();
	for (int i = 0; i < NUM_INCREMENTS; ++i) {
		++(*v);
	}
	//mx.unlock();
}
void dec() {
	
}

int main() {
	//mx.lock();
#ifdef USE_HEAP
	int * mem = new int[1];
	*mem = 0;
#else
	int var = 0;
	int * mem = &var;
#endif
	//mx.unlock();

	//auto ta = std::thread(&inc, mem);

	inc(mem);
	//auto tb = std::thread(&inc, mem);
	//
	//ta.join();
	//tb.join();
	
	//mx.lock();
	std::cout << "EXPECTED: " << 2 * NUM_INCREMENTS << ", "
		<< "ACTUAL: " << *mem << ", "
		<< "LOCATION: " << std::hex << (uint64_t) (mem) << std::endl;
	//mx.unlock();
#ifdef USE_HEAP
	delete mem;
#endif
	return 0;
}
