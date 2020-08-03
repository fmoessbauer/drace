#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2020 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "aligned-buffer.h"
#include "shadow-stack.h"
#include "statistics.h"

#include <dr_api.h>
#include <hashtable.h>

#include <atomic>
#include <type_traits>

namespace drace {

/**
 * \brief Per Thread data (thread-private)
 */
class ShadowThreadState {
 public:
  /**
   * \brief Pointer to the head of the memory buffer or null
   *
   * If the detector is disabled, this pointer is set to null
   */
  byte* buf_ptr;

  /**
   * \brief negative address of the end-pointer of the memory buffer
   *
   * As we use LEA + jmpecxz, the address has to be negated as we want to flush
   * the buffer if \ref buf_ptr and \ref buf_end sum up to zero.
   */
  ptr_int_t buf_end;

  /**
   * \brief Stores the \ref buf_ptr while the detector is disabled
   */
  byte* buf_ptr_stored;

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

  /// book-keeping of active mutexes
  hashtable_t mutex_book;

  /// ShadowStack
  ShadowStack stack;

  /// Statistics
  Statistics stats;

 public:
  explicit ShadowThreadState(void* drcontext);
  ~ShadowThreadState();
};

// this type trait is not correctly implemented by all compilers / stdlibs
// static_assert(std::is_standard_layout<ShadowThreadState>::value,
//              "ShadowThreadState has to be POD to be safely accessed via "
//              "inline assembly");

}  // namespace drace
