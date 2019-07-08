/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <thread>

int32_t global_uninit;            // goes into bss section
int32_t global_init = 0x12345678; // goes into data section

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
    printf("Stack @%p\n"
           "Text @%p\n"
           "Data @%p\n"
           "BSS @%p\n",
            &s, &main, &global_init, &global_uninit);
	fflush(stdout);

	auto ta = std::thread(fun, &s);
	auto tb = std::thread(fun2, &s);
	ta.join();
	tb.join();

	return 0;
}
