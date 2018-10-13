#include "Controller.h"
#include "ManagedResolver.h"
#include "ProtocolHandler.h"
#include "ipc/SyncSHMDriver.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

std::shared_ptr<spdlog::logger> logger;
std::unique_ptr<msr::ProtocolHandler> phandler;
std::shared_ptr<msr::Controller> controller;
/* future for controller loop */
std::future<void> ctl_fut;

/* Handle Ctrl Events / Signals */
BOOL CtrlHandler(DWORD fdwCtrlType) {
	if (nullptr == phandler.get())
		return false;

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

void printhelp() {
	std::cout << "Change detector state using the following keys:" << std::endl
		<< "e\tenable detector" << std::endl
		<< "d\tdisable detector" << std::endl
		<< "s\tset sampling rate" << std::endl;
}

int main(int argc, char** argv) {
	using namespace msr;

	logger = spdlog::stdout_logger_st("console");
	logger->set_pattern("[%Y-%m-%d %T][%l] %v");
	logger->set_level(spdlog::level::info);

	logger->info("Starting Managed Stack Resolver, waiting for Drace");
	if (argc == 2 && !strncmp(argv[1], "-v", 2)) {
		logger->set_level(spdlog::level::debug);
		logger->debug("Debug mode enabled");
	}

	// We need this event handler for cleanup of shm
	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
		logger->warn("Could not register exit handler");
	}

	printhelp();

	/*
	* The general idea here is to wait until DRACE
	* sets the pid in the shared memory.
	* Hence, msr can be started prior to drace
	*/
	try {
		// create memory for DRace controlblock
		auto drace_cb = std::make_shared<ipc::SharedMemory<ipc::ClientCB, false>>(DRACE_SMR_CB_NAME, true);
		controller = std::make_shared<Controller>(drace_cb);
		ctl_fut = std::async(std::launch::async, [=]() {controller->start();});

		// create driver + block for message passing
		auto msrdriver = std::make_shared<ipc::SyncSHMDriver<false, false>>(DRACE_SMR_NAME, true);
		phandler = std::make_unique<ProtocolHandler>(msrdriver);
		phandler->process_msgs();
	}
	catch (std::runtime_error e) {
		logger->error("Error initialising shared memory or during protocol: {}", e.what());
		return 1;
	}

	logger->info("Quit Managed Stack Resolver");
}
