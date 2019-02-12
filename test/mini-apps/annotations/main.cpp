#include <thread>

// Set this define to enable annotations.
// we do that using CMake to cross test in
// integration tests
//#define DRACE_ANNOTATION
#include "../../../drace-client/include/annotations/drace_annotation.h"

// This code serves as a test for client annotations

void inc(int * v) {
	for (int i = 0; i < 1000; ++i) {
		DRACE_ENTER_EXCLUDE();
		// This is racy, but we whitelist it
		int val = *v;
		++val;
		std::this_thread::yield();
		*v = val;
		DRACE_LEAVE_EXCLUDE();
	}

	DRACE_HAPPENS_BEFORE(v);
	*v++;
	DRACE_HAPPENS_AFTER(v);

}

int main() {
	int var = 0;

	auto ta = std::thread(&inc, &var);
	auto tb = std::thread(&inc, &var);

	ta.join();
	tb.join();

	return var;
}
