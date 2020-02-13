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

#include <atomic>
#include <memory>

// forward decls
class Detector;

/// DRace instrumentation framework
namespace drace {

/// File-based configuration
extern InstrumentationConfig config;
/// Runtime / CLI configuration
extern RuntimeConfig params;

/// id of master thread
extern std::atomic<unsigned> runtime_tid;

namespace module {
class Tracker;
}
/// Global Module Shadow Data
extern std::unique_ptr<module::Tracker> module_tracker;

class MemoryTracker;
/// Memory tracing subsystem
extern std::unique_ptr<MemoryTracker> memory_tracker;

/// Detector instance
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
/// shared memory driver for communication between drace and msr
extern std::unique_ptr<::ipc::MtSyncSHMDriver<true, true>> shmdriver;
/// external control-block to configure drace during execution
extern std::unique_ptr<::ipc::SharedMemory<ipc::ClientCB, true>> extcb;
}  // namespace drace
#endif
