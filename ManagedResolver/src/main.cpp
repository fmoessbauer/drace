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

/**
\brief Managed Symbol Resolver (MSR)
*/

#include "Controller.h"
#include "ManagedResolver.h"
#include "ProtocolHandler.h"
#ifdef EXTSAN
#include "QueueHandler.h"
#endif
#include "ipc/ExtsanData.h"
#include "ipc/SyncSHMDriver.h"
#include "version/version.h"

// clang-format off
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
// clang-format on

#include "clipp.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

std::shared_ptr<spdlog::logger> logger;
std::unique_ptr<msr::ProtocolHandler> phandler;
std::shared_ptr<msr::Controller> controller;
/* future for controller loop */
std::future<void> ctl_fut;

#ifdef EXTSAN
std::shared_ptr<msr::QueueHandler> qhandler;
std::future<void> q_fut;
std::future<void> qm_fut;  // Queue Monitor
#endif

/* Handle Ctrl Events / Signals */
BOOL CtrlHandler(DWORD fdwCtrlType) {
  if (nullptr == phandler.get()) return false;

  switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      // cleanup
      controller->stop();
      controller.reset();
      phandler.reset();
      return true;
  }
  return false;
}

int main(int argc, char** argv) {
  using namespace msr;

  logger = spdlog::stdout_logger_st("console");
  logger->set_pattern("[%Y-%m-%d %T][%l] %v");
  logger->set_level(spdlog::level::info);

  int loglevel = 1;
  bool display_help = false;
  bool exec_once = false;
  auto cli =
      (clipp::repeatable(
           clipp::option("-v", "--verbose")(clipp::increment(loglevel))) %
           "verbose, use multiple times to increase log-level (e.g. -v -v)",
       (clipp::option("--once").set(exec_once) % "exit after DRace finishes"),
       (clipp::option("--version")([]() {
         std::cout << "Managed Symbol Resolver (MSR)\n"
                   << "Version: " << DRACE_BUILD_VERSION << "\n"
                   << "Hash:    " << DRACE_BUILD_HASH << std::endl;
         std::exit(0);
       })) %
           "display version information",
       clipp::option("-h", "--usage").set(display_help));

  if (!clipp::parse(argc, argv, cli) || display_help) {
    std::cout << clipp::make_man_page(cli, "msr.exe") << std::endl;
    std::exit(display_help ? 0 : 1);
  }

  logger->info("Starting Managed Stack Resolver, waiting for Drace");

  auto level = spdlog::level::trace;
  std::string level_str = "trace";
  switch (loglevel) {
    case 0:
      level = spdlog::level::critical;
      level_str = "critical";
      break;
    case 1:
      level = spdlog::level::warn;
      level_str = "warning";
      break;
    case 2:
      level = spdlog::level::info;
      level_str = "info";
      break;
    case 3:
      level = spdlog::level::debug;
      level_str = "debug";
      break;
  }
  logger->info("Set loglevel to {}", level_str);
  logger->set_level(level);

  // We need this event handler for cleanup of shm
  if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
    logger->warn("Could not register exit handler");
  }

  /*
   * The general idea here is to wait until DRACE
   * sets the pid in the shared memory.
   * Hence, msr can be started prior to drace
   */
  try {
    // create memory for DRace controlblock
    auto drace_cb = std::make_shared<ipc::SharedMemory<ipc::ClientCB, false>>(
        DRACE_SMR_CB_NAME, true);
    if (!exec_once) {
      controller = std::make_shared<Controller>(drace_cb);
      ctl_fut = std::async(std::launch::async, [=]() { controller->start(); });
    }

#ifdef EXTSAN
    // Event message queue
    auto shm_queue =
        std::make_shared<ipc::SharedMemory<ipc::QueueMetadata, false>>(
            "drace-events", true);
    qhandler = std::make_shared<QueueHandler>(*(shm_queue->get()));
    q_fut = std::async(std::launch::async, [=]() { qhandler->start(); });
    qm_fut = std::async(std::launch::async, [=]() { qhandler->monitor(); });
#endif

    // create driver + block for message passing
    auto msrdriver = std::make_shared<ipc::SyncSHMDriver<false, false>>(
        DRACE_SMR_NAME, true);
    phandler = std::make_unique<ProtocolHandler>(msrdriver, exec_once);
    phandler->process_msgs();
  } catch (const std::runtime_error& e) {
    logger->error("Error initialising shared memory or during protocol: {}",
                  e.what());
    return 1;
  }

  logger->info("Quit Managed Stack Resolver");
}
