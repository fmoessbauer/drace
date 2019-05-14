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

#include <vector>
#include <iostream>
#include <iomanip>
#include <random>
#include <thread>
#include <atomic>

/// try to force a data-race
void inc(uint64_t * v, std::atomic<bool> * running) {
    while(running->load(std::memory_order_relaxed)){
        uint64_t var = *v;
        ++var;
        std::this_thread::yield();
        *v = var;
    }
}

/// calculate XOR hash of memory range
uint64_t get_hash(const uint64_t * begin, const uint64_t * end) {
    uint64_t hash = *begin;
    const uint64_t * item = begin;

    while (item != end) {
        hash ^= *item;
        ++item;
    }
    return hash;
}

/** 
* Test tool to check for memory corruption and layout.
* To also check the race reporting, we try to enforce data-races
*/
int main(int argc, char ** argv) {
    std::mt19937_64 gen(42);

    std::vector<std::pair<uint64_t*, uint64_t*>> blocks;
    std::vector<uint64_t>  checksums;
    size_t elem_allocated = 0;

    std::atomic<bool> keep_running{ true };
    uint64_t * mem = new uint64_t;
    *mem = 0ull;

    auto ta = std::thread(&inc, mem, &keep_running);
    auto tb = std::thread(&inc, mem, &keep_running);

    // generate blocks
    for (int i = 0; i < 32; ++i) {
        for (int i = 2; i <= (4096 * 1024); i *= 2) {
            uint64_t * ptr = new uint64_t[i];

            // fill block
            std::generate(ptr, ptr + i, [&]() {return gen(); });

            // compute and store checksum
            uint64_t checksum = get_hash(ptr, ptr + i);
            blocks.emplace_back(ptr, ptr + i);
            checksums.push_back(checksum);
            elem_allocated += i;
            std::cout << "Checksum of block " << std::setw(7) << i << " : " << (void*)checksum
                << " at " << (void*)ptr << std::endl;
        }
    }

    keep_running.store(false, std::memory_order_relaxed);
    ta.join();
    tb.join();

    auto alloc_mb = (elem_allocated * sizeof(uint64_t)) / (1024 * 1024);
    std::cout << "Allocated " << alloc_mb << " MiB" << std::endl;

    // validate checksums
    for (int i = 0; i < blocks.size(); ++i) {
        uint64_t checksum = get_hash(blocks[i].first, blocks[i].second);
        if (checksum != checksums[i]) {
            std::cout << "Mismatch in checksum of block " << i << std::endl;
            // exit and let os cleanup memory
            return 1;
        }
    }

    std::cout << "All blocks are valid" << std::endl;
    // let OS cleanup memory
}
