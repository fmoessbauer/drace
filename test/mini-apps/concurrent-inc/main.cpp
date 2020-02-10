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

#include <iostream>
#include <mutex>
#include <thread>

#define NUM_INCREMENTS 10000
#define USE_HEAP

// std::mutex mx;

void inc(int* v) {
  for (int i = 0; i < NUM_INCREMENTS; ++i) {
    int var = *v;
    ++var;
    std::this_thread::yield();
    *v = var;
  }
}
void dec(int* v) {
  for (int i = 0; i < NUM_INCREMENTS; ++i) {
    int var = *v;
    --var;
    std::this_thread::yield();
    *v = var;
  }
}

int main() {
#ifdef USE_HEAP
  int* mem = new int[1];
  *mem = 0;
#else
  int var = 0;
  int* mem = &var;
#endif

  auto ta = std::thread(&inc, mem);
  auto tb = std::thread(&dec, mem);

  ta.join();
  tb.join();

  // mx.lock();
  std::cout << "EXPECTED: " << 0 << ", "
            << "ACTUAL: " << *mem << ", "
            << "LOCATION: " << std::hex << (uintptr_t)(mem) << std::endl;
  // mx.unlock();
#ifdef USE_HEAP
  delete[] mem;
#endif
  return 0;
}
