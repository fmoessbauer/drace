#include <stdio.h>
#include <thread>

void fun() {
	printf("Hello from Thread 1\n");
	fflush(stdout);
}
void fun2() {
	printf("Hello from Thread 2\n");
	fflush(stdout);
}

int main(int argc, char ** argv) {
	printf("Hello World\n");
	fflush(stdout);

	//auto ta = std::thread(fun);
	//ta.join();
	
	//auto tb = std::thread(fun2);
	//tb.join();

	return 0;
}
