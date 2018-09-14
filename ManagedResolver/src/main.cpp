#include "ManagedResolver.h"
#include "ProtocolHandler.h"
#include "ipc/msr-driver.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

std::shared_ptr<spdlog::logger> logger;
std::unique_ptr<ProtocolHandler> phandler;

/* Handle Ctrl Events / Signals */
BOOL CtrlHandler(DWORD fdwCtrlType) {
	if (nullptr == phandler.get())
		return false;

	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		phandler->quit();
		return true;
	}
	return false;
}

int main(int argc, char** argv) {
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

	/*
	* The general idea here is to wait until DRACE
	* sets the pid in the shared memory.
	* Hence, msr can be started prior to drace
	*/
	try {
		auto msrdriver = std::make_shared<MsrDriver<false, false>>(DRACE_SMR_NAME, true);
		phandler = std::make_unique<ProtocolHandler>(msrdriver);
		phandler->process_msgs();
	}
	catch (std::runtime_error e) {
		logger->error("Error initialising shared memory or during protocol: {}", e.what());
		return 1;
	}

	logger->info("Quit Managed Stack Resolver");
}
