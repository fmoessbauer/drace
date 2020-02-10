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

#include "globals.h"
#include "DrFile.h"
#include "Module.h"
#include "memory-tracker.h"
#include "race-collector.h"
#include "statistics.h"
#include "symbols.h"
#if WIN32
#include "ipc/MtSyncSHMDriver.h"
#include "ipc/SharedMemory.h"
#endif
#include "util/DrModuleLoader.h"

#include <detector/Detector.h>

namespace drace {
/**
 * Thread local storage metadata has to be globally accessable
 */
int tls_idx;
std::unordered_map<thread_id_t, per_thread_t *> TLS_buckets;

void *th_mutex;
void *tls_rw_mutex;

// Global Config Object
drace::Config config;

std::atomic<int> num_threads_active{0};
std::atomic<uint> runtime_tid{0};

std::chrono::system_clock::time_point app_start;
std::chrono::system_clock::time_point app_stop;

std::unique_ptr<MemoryTracker> memory_tracker;
std::unique_ptr<module::Tracker> module_tracker;
std::unique_ptr<RaceCollector> race_collector;
std::shared_ptr<DrFile> log_file;
std::unique_ptr<Statistics> stats;

#if WIN32
std::unique_ptr<ipc::MtSyncSHMDriver<true, true>> shmdriver;
std::unique_ptr<ipc::SharedMemory<ipc::ClientCB, true>> extcb;
#endif

/* Runtime parameters */
params_t params;

std::unique_ptr<Detector> detector;

std::unique_ptr<::drace::util::DrModuleLoader> module_loader;
}  // namespace drace
