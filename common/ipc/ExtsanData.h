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

#include <array>

#include "ringbuffer.hpp"

namespace ipc {
/// Wrapped detector event for serializing
namespace event {
enum class Type : uint8_t {
  NONE,
  MEMREAD,
  MEMWRITE,
  ACQUIRE,
  RELEASE,
  HAPPENSBEFORE,
  HAPPENSAFTER,
  ALLOCATION,
  FREE,
  FORK,
  JOIN,
  DETACH,
  FINISH,
  FUNCENTER,
  FUNCEXIT
};

struct MemAccess {
  uint32_t thread_id;
  uintptr_t pc;
  uintptr_t addr;
  uintptr_t size;
};

struct Mutex {
  uint32_t thread_id;
  uintptr_t addr;
  int recursive;
  bool write;
  bool acquire;
};

struct HappensRelation {
  uint32_t thread_id;
  uintptr_t id;
};

struct Allocation {
  uint32_t thread_id;
  uintptr_t pc;
  uintptr_t addr;
  uintptr_t size;
};

struct ForkJoin {
  uint32_t parent;
  uint32_t child;
};

struct DetachFinish {
  uint32_t thread_id;
};

struct FuncEnter {
  uint32_t thread_id;
  uintptr_t pc;
};

struct FuncExit {
  uint32_t thread_id;
};

struct BufferEntry {
  Type type{Type::NONE};
  union {
    MemAccess memaccess;
    Mutex mutex;
    HappensRelation happens;
    Allocation allocation;
    ForkJoin forkjoin;
    DetachFinish detachfinish;
    FuncEnter funcenter;
    FuncExit funcexit;
  } payload;
};

template <typename T, class... Args>
struct GenericEntry {
  Type type{Type::NONE};
  T buffer;

  GenericEntry(Type t, Args&&... args)
      : type(t), buffer(std::forward<Args>(args)...) {}
};
}  // namespace event

using queue_t = ipc::Ringbuffer<ipc::event::BufferEntry, 1 << 20, false, 64>;

struct QueueMetadata {
  static constexpr unsigned num_queues{2};
  unsigned next_queue{0};
  queue_t queues[num_queues];
};
}  // namespace ipc
