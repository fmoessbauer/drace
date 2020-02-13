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

#include <dr_api.h>
#include <drmgr.h>
#include <cstring>
#include <memory>

namespace drace {

/**
 * \brief Wrapper around the thread-local storage of dynamorio
 */
template <typename T>
class DrThreadLocal {
  /// invalid tls id
  static constexpr int TLS_IDX_INVALID = -1;

 public:
  DrThreadLocal() = default;
  DrThreadLocal(const DrThreadLocal&) = delete;
  DrThreadLocal(DrThreadLocal&&) = default;
  ~DrThreadLocal() { freeAnyData(); }

  /**
   * \brief initialize the TLS on the calling thread
   *
   * and construct the object of type T in a thread-local buffer
   */
  template <typename... Args>
  T& addSlot(void* drcontext, Args&&... args) {
    ensureIsInitialized();
    ensureDrContextIsSet(drcontext);

    auto* mem = dr_thread_alloc(drcontext, sizeof(T));
    T* data = new (mem) T(std::forward<Args&&>(args)...);
    DR_ASSERT(mem != NULL);
    drmgr_set_tls_field(drcontext, _tlsIdx, mem);
    return *data;
  }

  void removeSlot(void* drcontext) const {
    ensureDrContextIsSet(drcontext);
    T* data = &getSlot(drcontext);
    data->~T();
    dr_thread_free(drcontext, data, sizeof(T));
  }

  /// get a reference to the locally allocated object
  inline T& getSlot(void* drcontext = nullptr) const {
    ensureDrContextIsSet(drcontext);

    auto* mem = drmgr_get_tls_field(drcontext, _tlsIdx);
    return *static_cast<T*>(mem);
  }

  bool freeAnyData() {
    bool res = false;
    if (_initialized) {
      res = drmgr_unregister_tls_field(_tlsIdx);
      _tlsIdx = 0;
      _initialized = false;
    }
    return res;
  }

  /// return the dynamorio tls identifier
  inline int getTlsIndex() const { return _tlsIdx; }

 private:
  void ensureIsInitialized() {
    if (!_initialized) {
      _tlsIdx = drmgr_register_tls_field();
      DR_ASSERT_MSG(_tlsIdx != TLS_IDX_INVALID, "could not register TLS");
      _initialized = true;
    }
  }

  static void ensureDrContextIsSet(void*& context) {
    if (context == nullptr) {
      context = dr_get_current_drcontext();
    }
  }

  int _tlsIdx{TLS_IDX_INVALID};
  bool _initialized{false};
};
}  // namespace drace
