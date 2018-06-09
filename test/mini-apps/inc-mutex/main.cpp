#include <thread>
#include <mutex>

// This code serves as a test for mutex detection
// in the drace client

static std::mutex mx;

void inc(int * v) {
	mx.lock();
	*v += 1;
	mx.unlock();
}

int main() {
	int var = 0;
	//auto ta = std::thread(&inc, &var);
	inc(&var);

	//ta.join();

	return var;
}
