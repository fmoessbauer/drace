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

#include "MSR.h"
#include "globals.h"
#include "ipc/MtSyncSHMDriver.h"
#include "ipc/SMData.h"

#include <mutex>

namespace drace {

void MSR::wait_heart_beat() {
  while (shmdriver->wait_receive(std::chrono::seconds(2)) &&
         shmdriver->id() == ipc::SMDataID::WAIT) {
    LOG_NOTICE(0, "wait for download to finish");
    shmdriver->id(ipc::SMDataID::CONFIRM);
    shmdriver->commit();
  }
}

bool MSR::connect() {
  using namespace std::chrono;
  constexpr seconds timeout(10);
  LOG_INFO(0, "wait %is for MPCR to connect", timeout.count());

  auto time_start = system_clock::now();
  while (system_clock::now() - time_start < timeout) {
    shmdriver = std::make_unique<ipc::MtSyncSHMDriver<true, true>>(
        DRACE_SMR_NAME, false);
    if (shmdriver->valid()) {
      shmdriver->wait_receive(seconds(10));
      if (shmdriver->id() == ipc::SMDataID::READY) {
        shmdriver->id(ipc::SMDataID::CONNECT);
        shmdriver->commit();
        // Got stable connection, attach to CB
        extcb = std::make_unique<ipc::SharedMemory<ipc::ClientCB, true>>(
            DRACE_SMR_CB_NAME, false);
        DR_ASSERT(extcb->get() != nullptr);
        // TODO: Error handling (rarely necessary)
        // Initialize extcb
        extcb->get()->sampling_rate.store(params.sampling_rate,
                                          std::memory_order_relaxed);
        break;
      } else {
        LOG_WARN(0, "MSR is not ready to connect");
        shmdriver.reset();
      }
    } else {
      LOG_NOTICE(0, "could not create SHM");
      shmdriver.reset();
    }
    LOG_NOTICE(0, "MPCR not started yet, try again in %is", 1);
    dr_sleep(1000);  // in ms
  }
  return static_cast<bool>(shmdriver);
}

bool MSR::attach(const module_data_t* mod) {
  DR_ASSERT(shmdriver != nullptr);
  LOG_INFO(0, "wait 10s for external resolver to attach");
  if (shmdriver) {
    shmdriver->wait_receive(std::chrono::seconds(10));
    if (shmdriver->id() == ipc::SMDataID::CONFIRM) {
      // Send PID and CLR path
      auto& sendInfo = shmdriver->emplace<ipc::BaseInfo>(ipc::SMDataID::ATTACH);
      sendInfo.pid = (int)dr_get_process_id();
      strncpy(sendInfo.path.data(), mod->full_path, sendInfo.path.size());
      shmdriver->commit();

      MSR::wait_heart_beat();
      switch (shmdriver->id()) {
        case ipc::SMDataID::ATTACHED:
          LOG_INFO(0, "MSR attached");
          break;
        default:
          LOG_WARN(0, "Protocol error, got %u", shmdriver->id());
          shmdriver.reset();
          break;
      }
      if (!shmdriver) {
        LOG_WARN(0, "MSR did not attach");
      }
    } else {
      LOG_WARN(0, "MSR is not ready to connect");
      shmdriver.reset();
    }
  } else {
    LOG_WARN(0, "MSR is not running");
    shmdriver.reset();
  }
  return static_cast<bool>(shmdriver);
}

bool MSR::request_symbols(const module_data_t* mod) {
  DR_ASSERT(shmdriver != nullptr);
  std::lock_guard<decltype(*shmdriver)> lg(*shmdriver);
  LOG_INFO(0,
           "MSR downloads the symbols from a symbol server (might take long)");
  // TODO: Download Symbols for some other .Net dlls as well
  auto& symreq =
      shmdriver->emplace<ipc::SymbolRequest>(ipc::SMDataID::LOADSYMS);
  symreq.base = (uint64_t)mod->start;
  symreq.size = mod->module_internal_size;
  strncpy(symreq.path.data(), mod->full_path, symreq.path.size());
  shmdriver->commit();

  MSR::wait_heart_beat();
  if (shmdriver->id() == ipc::SMDataID::CONFIRM) {
    LOG_INFO(0, "Symbols downloaded");
    return true;
  } else {
    return false;
  }
}

void MSR::unload_symbols(app_pc mod_start) {
  DR_ASSERT(shmdriver != nullptr);
  std::lock_guard<decltype(*shmdriver)> lg(*shmdriver);
  auto& sr = shmdriver->emplace<ipc::SymbolRequest>(ipc::SMDataID::UNLOADSYMS);
  sr.base = (uint64_t)mod_start;
  shmdriver->commit();
  shmdriver->wait_receive();
  DR_ASSERT(shmdriver->id() == ipc::SMDataID::CONFIRM);
  LOG_NOTICE(-1, "Closed Symbols");
}

ipc::SymbolInfo MSR::lookup_address(app_pc pc) {
  DR_ASSERT(shmdriver != nullptr);
  std::lock_guard<decltype(*shmdriver)> lg(*shmdriver);
  shmdriver->put<uint64_t>(ipc::SMDataID::IP, (uint64_t)pc);
  shmdriver->commit();
  if (shmdriver->wait_receive(std::chrono::seconds(100))) {
    return shmdriver->get<ipc::SymbolInfo>();
  }
  LOG_WARN(0, "Timeout expired");
  return ipc::SymbolInfo();
}

ipc::SymbolResponse MSR::search_symbol(const module_data_t* mod,
                                       const std::string& match,
                                       bool full_search) {
  DR_ASSERT(shmdriver != nullptr);
  std::lock_guard<decltype(*shmdriver)> lg(*shmdriver);
  auto& sr = shmdriver->emplace<ipc::SymbolRequest>(ipc::SMDataID::SEARCHSYMS);
  sr.base = (uint64_t)mod->start;
  sr.size = mod->module_internal_size;
  sr.full = full_search;
  strncpy(sr.path.data(), mod->full_path, sr.path.size());
  DR_ASSERT_MSG(match.size() <= sr.match.size(), "Matchstr larger than buffer");
  std::copy(match.begin(), match.end(), sr.match.begin());
  shmdriver->commit();
  if (shmdriver->wait_receive(std::chrono::seconds(100))) {
    return shmdriver->get<ipc::SymbolResponse>();
  }
  LOG_WARN(0, "Timeout expired");
  return ipc::SymbolResponse();
}

void MSR::getCurrentStack(int threadid, void* rbp, void* rsp, void* rip) {
  DR_ASSERT(shmdriver != nullptr);
  auto& mc = shmdriver->emplace<ipc::MachineContext>(ipc::SMDataID::STACK);
  mc.threadid = threadid;
  mc.rbp = (uint64_t)rbp;
  mc.rsp = (uint64_t)rsp;
  mc.rip = (uint64_t)rip;
  shmdriver->commit();
  shmdriver->wait_receive();
}

}  // namespace drace
