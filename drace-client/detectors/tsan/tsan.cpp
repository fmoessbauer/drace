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

#include <map>
#include <vector>
#include <atomic>
#include <algorithm>
#include <mutex> // for lock_guard
#include <unordered_map>
#include <iostream>
#include <cassert>

#include <detector/detector_if.h>

#include <ipc/spinlock.h>
#include <tsan-if.h>

namespace detector {
    /// tsan detector based on LLVM ThreadSanitizer
    namespace tsan {

        struct tsan_params_t {
            bool heap_only{ false };
        } params;

        struct ThreadState {
            detector::tls_t tsan;
            bool active;
        };

        /// reserved area at begin of shadow memory addr range;
        constexpr int MEMSTART = 4096;

        /**
         * To avoid false-positives track races only if they are on the heap
         * invert order to get range using lower_bound
         */
        static std::atomic<bool>		alloc_readable{ true };
        static std::atomic<uint64_t>	misses{ 0 };
        static std::atomic<uint64_t>    races{ 0 };
        // lower bound of heap
        static std::atomic<uint64_t>    heap_lb{ std::numeric_limits<uint64_t>::max() };
        // upper bound of heap
        static std::atomic<uint64_t>    heap_ub{ 0 };
        /* Cannot use std::mutex here, hence use spinlock */
        static ipc::spinlock            mxspin;
        static std::map<uint64_t, size_t, std::greater<uint64_t>> allocations;
        static std::unordered_map<detector::tid_t, ThreadState> thread_states;

        void reportRaceCallBack(__tsan_race_info* raceInfo, void * add_race_clb) {
            // Fixes erronous thread exit handling by ignoring races where at least one tid is 0
            if (!raceInfo->access1->user_id || !raceInfo->access2->user_id)
                return;

            detector::Race race;

            // TODO: Currently we assume that there is at most one callback at a time
            __tsan_race_info_access * race_info_ac;
            for (int i = 0; i != 2; ++i) {
                detector::AccessEntry access;
                if (i == 0) {
                    race_info_ac = raceInfo->access1;
                    races.fetch_add(1, std::memory_order_relaxed);
                }
                else {
                    race_info_ac = raceInfo->access2;
                }

                access.thread_id = race_info_ac->user_id;
                access.write = race_info_ac->write;
                access.accessed_memory = (uint64_t)race_info_ac->accessed_memory;
                access.access_size = race_info_ac->size;
                access.access_type = race_info_ac->type;

                size_t ssize = std::min(race_info_ac->stack_trace_size, detector::max_stack_size);
                memcpy(access.stack_trace, race_info_ac->stack_trace, ssize * sizeof(uint64_t));
                access.stack_size = ssize;

                uint64_t addr = (uint64_t)(race_info_ac->accessed_memory);
                // todo: lock allocations
                mxspin.lock();
                auto it = allocations.lower_bound(addr);
                if (it != allocations.end() && (addr < (it->first + it->second))) {
                    access.onheap = true;
                    access.heap_block_begin = it->first;
                    access.heap_block_size = it->second;
                }
                mxspin.unlock();

                if (i == 0) {
                    race.first = access;
                }
                else {
                    race.second = access;
                }
            }
            ((void(*)(const detector::Race*))add_race_clb)(&race);
        }

        /*
         * \brief split address at 32-bit boundary (zero above)
         *
         * TSAN internally uses a shadow memory block to track all
         * accesses. Here, we use a fixed 32-bit block and map
         * our accesses to this block by just considering the lower
         * half. This leads to false positives, if the mapping is not
         * unique. However even with extensive tests on Windows, we
         * never discovered such a situation.
         *
         * TODO: Find a solution to reliably detect all stack and heap
         *       regions to finally work with full 64-bit addresses
         */
        constexpr uint64_t lower_half(uint64_t addr) {
            return (addr & 0x00000000FFFFFFFF);
        }

        /** Fast trivial hash function using a prime. We expect tids in [1,10^5] */
        constexpr uint64_t get_event_id(detector::tid_t parent, detector::tid_t child) {
            return parent * 65521 + child;
        }
        constexpr uint64_t get_event_id(detector::tls_t parent, detector::tid_t child) {
            // not ideal as symmetric
            return (uint64_t)(parent)+child;
        }
        constexpr uint64_t get_event_id(detector::tls_t parent, detector::tls_t child) {
            // not ideal as symmetric
            return (uint64_t)(parent) ^ (uint64_t)(child);
        }

        static void parse_args(int argc, const char ** argv) {
            int processed = 1;
            while (processed < argc) {
                if (strncmp(argv[processed], "--heap-only", 16) == 0) {
                    params.heap_only = true;
                    ++processed;
                }
                else {
                    ++processed;
                }
            }
        }
        static void print_config() {
            std::cout << "> Detector Configuration:\n"
                << "> heap-only: " << (params.heap_only ? "ON" : "OFF") << std::endl
                << "> version:   " << detector::version() << std::endl;
        }

        /* precisely decide if (cropped to 32 bit) addr is on the heap */
        template<bool fastapprox = false>
        static inline bool on_heap(uint64_t addr) {
            // filter step without locking
            if (on_heap<true>(addr)) {
                std::lock_guard<spinlock> lg(mxspin);

                auto it = allocations.lower_bound(addr);
                uint64_t beg = it->first;
                size_t   sz = it->second;

                return (addr < (beg + sz));
            }
            else {
                return false;
            }
        }

        /* approximate if (cropped to 23 bit) addr is on the heap
        *  (no false-negatives, but possibly false positives)
        */
        template<>
        static inline bool on_heap<true>(uint64_t addr) {
            return ((addr >= heap_lb.load(std::memory_order_relaxed))
                && (addr < heap_ub.load(std::memory_order_relaxed)));
        }

    } // namespace tsan
} // namespace detector

using namespace detector::tsan;

bool detector::init(int argc, const char **argv, Callback rc_clb) {
    parse_args(argc, argv);
    print_config();

    thread_states.reserve(128);

    __tsan_init_simple(reportRaceCallBack, (void*)rc_clb);
    // we create a shadow memory of nearly the whole 32-bit address range
    // all memory accesses have to be inside this region
    __tsan_map_shadow((void*)MEMSTART, (size_t)(0xFFFFFFFF - MEMSTART));
    return true;
}

void detector::finalize() {
    for (auto & t : thread_states) {
        if (t.second.active)
            detector::finish(t.second.tsan, t.first);
    }
    // TODO: this calls exit which we cannot do here
    //__tsan_fini();
    // do not perform IO here, as it crashes / interfers with dotnet
    //std::cout << "> ----- SUMMARY -----" << std::endl;
    //std::cout << "> Found " << races.load(std::memory_order_relaxed) << " possible data-races" << std::endl;
    //std::cout << "> Detector missed " << misses.load() << " possible heap refs" << std::endl;
}

std::string detector::name() {
    return std::string("TSAN");
}

std::string detector::version() {
    return std::string("0.2.0");
}

void detector::func_enter(tls_t tls, void* pc) {
    __tsan_func_enter(tls, pc);
}

void detector::func_exit(tls_t tls) {
    __tsan_func_exit(tls);
}

void detector::acquire(tls_t tls, void* mutex, int rec, bool write) {
    uint64_t addr_32 = lower_half((uint64_t)mutex);
    // if the mutex is a handle, the ID might be too small
    if (addr_32 < MEMSTART)
        addr_32 += MEMSTART;

    //std::cout << "detector::acquire " << thread_id << " @ " << mutex << std::endl;

    assert(nullptr != tls);

    __tsan_mutex_after_lock(tls, (void*)addr_32, (void*)write);
}

void detector::release(tls_t tls, void* mutex, bool write) {
    uint64_t addr_32 = lower_half((uint64_t)mutex);
    // if the mutex is a handle, the ID might be too small
    if (addr_32 < MEMSTART)
        addr_32 += MEMSTART;

    //std::cout << "detector::release " << thread_id << " @ " << mutex << std::endl;

    assert(nullptr != tls);

    __tsan_mutex_before_unlock(tls, (void*)addr_32, (void*)write);
}

void detector::happens_before(tls_t tls, void* identifier) {
    uint64_t addr_32 = lower_half((uint64_t)identifier);
    __tsan_happens_before(tls, (void*)addr_32);
}

void detector::happens_after(tls_t tls, void* identifier) {
    uint64_t addr_32 = lower_half((uint64_t)identifier);
    __tsan_happens_after(tls, (void*)addr_32);
}

void detector::read(tls_t tls, void* pc, void* addr, size_t size)
{
    uint64_t addr_32 = lower_half((uint64_t)addr);
    if (!params.heap_only || on_heap<true>((uint64_t)addr_32)) {
        __tsan_read(tls, (void*)addr_32, (void*)pc);
    }
}

void detector::write(tls_t tls, void* pc, void* addr, size_t size)
{
    uint64_t addr_32 = lower_half((uint64_t)addr);

    if (!params.heap_only || on_heap<true>((uint64_t)addr_32)) {
        __tsan_write(tls, (void*)addr_32, (void*)pc);
    }
}

void detector::allocate(tls_t tls, void* pc, void* addr, size_t size) {
    uint64_t addr_32 = lower_half((uint64_t)addr);

    __tsan_malloc(tls, pc, (void*)addr_32, size);

    {
        std::lock_guard<ipc::spinlock> lg(mxspin);
        allocations.emplace(addr_32, size);
    }

    //std::cout << "alloc: addr: " << (void*)addr_32 << " size " << size << std::endl;

    // this is a bit racy as other allocations might finish first
    // but this is ok as only approximations are necessary

    // increase heap upper bound
    uint64_t new_ub = addr_32 + size;
    if (new_ub > heap_ub.load(std::memory_order_relaxed)) {
        heap_ub.store(new_ub, std::memory_order_relaxed);
        //std::cout << "New heap ub " << std::hex << new_ub << std::endl;
    }
    // decrease heap lower bound
    if (addr_32 < heap_lb.load(std::memory_order_relaxed)) {
        heap_lb.store(addr_32, std::memory_order_relaxed);
        //std::cout << "New heap lb " << std::hex << addr_32 << std::endl;
    }

}

void detector::deallocate(tls_t tls, void* addr) {
    uint64_t addr_32 = lower_half((uint64_t)addr);

    mxspin.lock();
    // ocasionally free is called more often than allocate, hence guard
    if (allocations.count(addr_32) == 1) {
        size_t size = allocations[addr_32];
        allocations.erase(addr_32);
        // if allocation was top of heap, decrease heap_limit
        if (addr_32 + size == heap_ub.load(std::memory_order_relaxed)) {
            if (allocations.size() != 0) {
                auto upper_heap = allocations.rbegin();
                heap_ub.store(upper_heap->first + upper_heap->second);
            }
            else {
                heap_ub.store(0, std::memory_order_relaxed);
            }
        }
        mxspin.unlock();

        __tsan_free((void*)addr_32, size);
        //std::cout << "free: addr:  " << (void*)addr_32 << " size " << size << std::endl;
    }
    else {
        // we expect some errors here, as either dr does not catch all 
        // alloc events, or they are not fully balanced.
        // TODO: compare with drmemory
        mxspin.unlock();
        //std::cout << "Error on free at " << (void*) addr_32 << std::endl;
    }
}

void detector::fork(tid_t parent, tid_t child, tls_t * tls) {
    const uint64_t event_id = get_event_id(parent, child);
    *tls = __tsan_create_thread(child);
   
    std::lock_guard<ipc::spinlock> lg(mxspin);

    for (const auto & t : thread_states) {
        if (t.second.active) {
            assert(nullptr != t.second.tsan);
            __tsan_happens_before(t.second.tsan, (void*)(event_id));
        }
    }
    __tsan_happens_after(*tls, (void*)(event_id));
    thread_states[child] = ThreadState{ *tls, true };
}

void detector::join(tid_t parent, tid_t child) {
    const uint64_t event_id = get_event_id(parent, child);

    std::lock_guard<ipc::spinlock> lg(mxspin);
    thread_states[child].active = false;

    const auto thr = thread_states[child].tsan;
    assert(nullptr != thr);
    __tsan_happens_before(thr, (void*)(event_id));
    for (const auto & t : thread_states) {
        if (t.second.active) {
            assert(nullptr != t.second.tsan);
            __tsan_happens_after(t.second.tsan, (void*)(event_id));
        }
    }
    // we cannot use __tsan_ThreadJoin here, as local tid is not tracked
    // hence use thread finish and draw edge between exited thread
    // and all other threads
    __tsan_ThreadFinish(thread_states[child].tsan);
}

void detector::detach(tls_t tls, tid_t thread_id) {
    // TODO: This is currently not supported
    return;
}

void detector::finish(tls_t tls, tid_t thread_id) {
    if (tls != nullptr) {
        __tsan_go_end(tls);
    }
    std::lock_guard<ipc::spinlock> lg(mxspin);
    thread_states[thread_id].active = false;
}
