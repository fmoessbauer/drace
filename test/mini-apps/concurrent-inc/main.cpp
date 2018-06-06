#include <thread>

void inc(int * v) {
	++(*v);
}

int main() {
	int var = 0;
	auto ta = std::thread(&inc, &var);
	inc(&var);

	ta.join();
	return var;
}
