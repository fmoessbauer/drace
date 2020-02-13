#pragma once
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

// Log up to notice (this parameter can be controlled using CMake)
#ifndef LOGLEVEL
#define LOGLEVEL 3
#endif

#include "InstrumentationConfig.h"
#include "RuntimeConfig.h"
#include "aligned-buffer.h"
#include "shadow-stack.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include <dr_api.h>
#include <hashtable.h>

/// max number of individual mutexes per thread
constexpr int MUTEX_MAP_SIZE = 128;

// forward decls
class Detector;

/// DRace instrumentation framework
namespace drace {

// Global Configuration
extern InstrumentationConfig config;
extern RuntimeConfig params;

// TODO check if global is better
extern std::atomic<uint> runtime_tid;

// Global Module Shadow Data
namespace module {
class Tracker;
}
extern std::unique_ptr<module::Tracker> module_tracker;

class MemoryTracker;
extern std::unique_ptr<MemoryTracker> memory_tracker;

// Detector instance
extern std::unique_ptr<Detector> detector;

}  // namespace drace

// MSR Communication Driver
namespace ipc {
template <bool, bool>
class MtSyncSHMDriver;

template <typename T, bool>
class SharedMemory;

struct ClientCB;
}  // namespace ipc

#if WIN32
// \todo currently only available on windows
namespace drace {
extern std::unique_ptr<::ipc::MtSyncSHMDriver<true, true>> shmdriver;
extern std::unique_ptr<::ipc::SharedMemory<ipc::ClientCB, true>> extcb;
}  // namespace drace
#endif
