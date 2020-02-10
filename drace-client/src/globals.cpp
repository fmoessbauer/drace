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
#include "Module.h"
#include "memory-tracker.h"
#include "statistics.h"
#include "symbols.h"
#if WIN32
#include "ipc/MtSyncSHMDriver.h"
#include "ipc/SharedMemory.h"
#endif

#include <detector/Detector.h>

namespace drace {
/**
 * Thread local storage metadata has to be globally accessable
 */
int tls_idx;

void *th_mutex;
void *tls_rw_mutex;

// Global Config Object
drace::Config config;

std::atomic<uint> runtime_tid{0};

std::unique_ptr<MemoryTracker> memory_tracker;
std::unique_ptr<module::Tracker> module_tracker;
std::unique_ptr<Statistics> stats;

#if WIN32
std::unique_ptr<ipc::MtSyncSHMDriver<true, true>> shmdriver;
std::unique_ptr<ipc::SharedMemory<ipc::ClientCB, true>> extcb;
#endif

/* Runtime parameters */
params_t params;

std::unique_ptr<Detector> detector;

}  // namespace drace
