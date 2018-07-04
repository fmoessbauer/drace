#include <thread>
#include <mutex>

// This code serves as a test for mutex detection
// in the drace client

static std::mutex mx;

void inc(int * v) {
	for (int i = 0; i < 100; ++i) {
		mx.lock();
		*v += 1;
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
