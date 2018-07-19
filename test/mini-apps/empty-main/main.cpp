#include <stdio.h>
#include <thread>

void fun(int * s) {
	for (int i = 0; i < 1000; ++i) {
		printf("Hello from Thread 1: Sence of Life: %i\n", *s);
		fflush(stdout);
	}

}
void fun2(int * s) {
	for (int i = 0; i < 1000; ++i) {
		printf("Hello from Thread 2: Sence of Life: %i\n", *s);
		fflush(stdout);
	}
}

int main(int argc, char ** argv) {
	int s = 42;
	printf("Hello World\n");
	fflush(stdout);

	auto ta = std::thread(fun, &s);
	auto tb = std::thread(fun2, &s);
	ta.join();
	tb.join();

	return 0;
}
