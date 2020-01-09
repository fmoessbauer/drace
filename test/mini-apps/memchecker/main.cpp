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
#include <mutex>
#include <algorithm>

#define DRACE_ANNOTATION
#include "../../../drace-client/include/annotations/drace_annotation.h"

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

void generate_block(
    int i,
    std::vector<std::pair<uint64_t*, uint64_t*>> * blocks,
    std::vector<uint64_t> * checksums)
{
    static std::mutex mx;
    std::mt19937_64 gen(42+i);

    uint64_t * ptr = new uint64_t[i];

    // fill block
    std::generate(ptr, ptr + i, [&]() {return gen(); });

    // compute and store checksum
    uint64_t checksum = get_hash(ptr, ptr + i);

    {
        mx.lock();
        blocks->emplace_back(ptr, ptr + i);
        checksums->push_back(checksum);
        std::cout << "Checksum of block " << std::setw(7) << i << " : " << (void*)checksum
            << " at " << (void*)ptr << std::endl;
        mx.unlock();
    }
}
/** 
* Test tool to check for memory corruption and layout.
* To also check the race reporting, we try to enforce data-races
*/
int main(int argc, char ** argv) {
    std::vector<std::pair<uint64_t*, uint64_t*>> blocks;
    std::vector<uint64_t>  checksums;
    std::vector<std::thread> workers;
    size_t elem_allocated = 0;

    // generate blocks
    workers.reserve(1000);
    for (int i = 0; i < 32; ++i) {
        for (int j = 2; j <= (4096 * 1024); j *= 2) {
            workers.emplace_back(generate_block, j, &blocks, &checksums);
            elem_allocated += j;
        }
    }

    for (auto & t : workers) {
        t.join();
    }

    auto alloc_mb = (elem_allocated * sizeof(uint64_t)) / (1024 * 1024);
    std::cout << "Allocated " << alloc_mb << " MiB, Threads: " << workers.size() << std::endl;

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
