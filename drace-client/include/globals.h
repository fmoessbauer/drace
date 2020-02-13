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

#include "aligned-buffer.h"
#include "config.h"
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

/** Upper limit of process address space according to
 *   https://docs.microsoft.com/en-us/windows/win32/memory/memory-limits-for-windows-releases#memory-limits-for-windows-and-windows-server-releases
 *   This also holds for Linux x64
 *   https://www.kernel.org/doc/Documentation/x86/x86_64/mm.txt
 */
#if COMPILE_X86
constexpr uintptr_t PROC_ADDR_LIMIT = 0xBFFFFFFF;
#else
constexpr uintptr_t PROC_ADDR_LIMIT = 0x00007FFF'FFFFFFFF;
#endif

// forward decls
class Detector;

/// DRace instrumentation framework
namespace drace {

/// Runtime parameters
struct params_t {
  unsigned sampling_rate{1};
  unsigned instr_rate{1};
  bool lossy{false};
  bool lossy_flush{false};
  bool excl_traces{false};
  bool excl_stack{false};
  bool exclude_master{false};
  bool delayed_sym_lookup{false};
  /// search for annotations in modules of target application
  bool annotations{true};
  unsigned suppression_level{1};
  /** Use external controller */
  bool extctrl{false};
  bool break_on_race{false};
  bool stats_show{false};
  unsigned stack_size{31};
  std::string config_file{"drace.ini"};
  std::string out_file;
  std::string xml_file;
  std::string logfile{"stderr"};
  std::string detector{"tsan"};
  std::string filter_file{"race_suppressions.txt"};

  // Raw arguments
  int argc;
  const char** argv;
};
extern params_t params;

class Statistics;

/**
 * \brief Per Thread data (thread-private)
 *
 * \warning Initialisation is done in the thread-creation event
 *          in \see MemoryTracker.
 *          Prior thread-creation events must not use this data
 */
struct per_thread_t {
  byte* buf_ptr;
  ptr_int_t buf_end;

  /// Represents the detector state.
  byte enabled{true};
  /// inverse of flush pending, jmpecxz
  std::atomic<byte> no_flush{false};
  /// local sampling state
  int sampling_pos = 0;

  thread_id_t tid;

  /// track state of detector (count nr of disable / enable calls)
  uintptr_t event_cnt{0};

  /// begin of this threads stack range
  uintptr_t appstack_beg{0x0};
  /// end of this threads stack range
  uintptr_t appstack_end{0x0};

  /**
   * as the detector cannot allocate TLS,
   * use this ptr for per-thread data in detector */
  void* detector_data{nullptr};

  /// buffer containing memory accesses
  AlignedBuffer<byte, 64> mem_buf;

  /// Statistics
  Statistics* stats;

  /// book-keeping of active mutexes
  hashtable_t mutex_book;

  /// ShadowStack
  ShadowStack stack;
};

static_assert(
    std::is_standard_layout<per_thread_t>::value,
    "per_thread_t has to be POD to be safely accessed via inline assembly");

// TODO check if global is better
extern std::atomic<uint> runtime_tid;

// Global Module Shadow Data
namespace module {
class Tracker;
}
extern std::unique_ptr<module::Tracker> module_tracker;

class MemoryTracker;
extern std::unique_ptr<MemoryTracker> memory_tracker;

// Global Configuration
extern drace::Config config;

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
