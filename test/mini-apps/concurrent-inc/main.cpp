#include <thread>
#include <iostream>

#define NUM_INCREMENTS 1'000'000

void inc(int * v) {
	for (int i = 0; i < NUM_INCREMENTS; ++i) {
		++(*v);
	}
}

int main() {
	int var = 0;
	auto ta = std::thread(&inc, &var);
	auto tb = std::thread(&inc, &var);

	ta.join();
	tb.join();

	std::cout << "EXPECTED: " << 2 * NUM_INCREMENTS << ", "
		<< "ACTUAL: " << var << std::endl;
	return 0;
}
