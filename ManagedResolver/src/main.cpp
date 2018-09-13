#include "ManagedResolver.h"
#include "ProtocolHandler.h"
#include "ipc/msr-driver.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

std::shared_ptr<MsrDriver<false, false>> msrdriver;
std::unique_ptr<ProtocolHandler> phandler;

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
	std::cout << "Starting Managed Stack Resolver, waiting for Drace" << std::endl;

	// We need this event handler for cleanup of shm
	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
		std::cerr << "Warning: Could not register exit handler" << std::endl;
	}

	/*
	* The general idea here is to wait until DRACE
	* sets the pid in the shared memory.
	* Hence, msr can be started prior to drace
	*/
	try {
		msrdriver = std::make_shared<MsrDriver<false, false>>(DRACE_SMR_NAME, true);
	}
	catch (std::runtime_error e) {
		std::cerr << "Error initialising shared memory: " << e.what() << std::endl;
	}

	phandler = std::make_unique<ProtocolHandler>(msrdriver);
	phandler->process_msgs();

	std::cout << "Quit Managed Stack Resolver" << std::endl;
}