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

#include <cstring>
#include <thread>

#include "SMData.h"
#include "SharedMemory.h"

namespace ipc {

/** Handles communication between Drace and MSR.
 * Provides methods to commit a write and wait for
 * data.
 * \note Not Threadsafe
 */
template <bool SENDER, bool NOTHROW = true>
class SyncSHMDriver {
 private:
  SharedMemory<SMData, NOTHROW> _shm;
  SMData* _comm;

 public:
  /// Attach to shared memory if any
  SyncSHMDriver(const char* shmkey, bool create)
      : _shm(shmkey, create), _comm(_shm.get()) {}

  ~SyncSHMDriver() {
    if (nullptr != _comm) {
      _comm->id = SMDataID::EXIT;
      commit();
    }
  }

  /** Returns a pointer to the raw buffer */
  template <typename T = byte>
  inline T* data() {
    return reinterpret_cast<T*>(_comm->buffer);
  }

  inline SMDataID id() const { return _comm->id; }

  inline void commit() { _shm.notify(); }

  template <typename duration = std::chrono::milliseconds>
  inline bool wait_receive(const duration& d = std::chrono::milliseconds(100)) {
    return _shm.wait(d);
  }

  /** Sets the message id / state */
  inline void id(SMDataID mid) { _comm->id = mid; }

  /** Puts value of type T at begin of buffer */
  template <typename T>
  inline void put(SMDataID mid, const T& val) {
    _comm->id = mid;
    reinterpret_cast<T*>(_comm->buffer)[0] = val;
  }

  /** Constructs T at begin of buffer */
  template <typename T, class... Args>
  inline T& emplace(SMDataID mid, Args&&... args) {
    _comm->id = mid;
    return *(new (_comm->buffer) T(std::forward<Args>(args)...));
  }

  /** Gets value of type T from begin of buffer */
  template <typename T>
  inline const T& get() const {
    return reinterpret_cast<T*>(_comm->buffer)[0];
  }

  /**
   * returns true if the msrdriver is in a valid state and can be used.
   * This is for the nothrow case, where the constructor cannot
   * throw a exception if an error occured during shm attaching
   */
  inline bool valid() const { return (nullptr != _comm); }
};

}  // namespace ipc
