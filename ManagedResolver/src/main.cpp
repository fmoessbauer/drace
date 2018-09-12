#include "ManagedResolver.h"
#include "ipc/SharedMemory.h"
#include "ipc/SMData.h"

#include <iostream>
#include <thread>
#include <chrono>

ManagedResolver resolver;
SMData *        comm;

void resolveIP() {
	CString symbol;
	std::cout << "Got PC: " << (void*)(comm->data.ip) << std::endl;
	resolver.GetMethodName((void*)comm->data.ip, symbol);
	char * symname = symbol.GetBuffer(DRACE_SMR_MAXLEN);
	strncpy(comm->data.symbol, symname, DRACE_SMR_MAXLEN);
	comm->ready.store(true, std::memory_order_release);
}

bool process_msg() {
	// wait for incoming requests
	do {
		bool trueval = true;
		if (comm->process.compare_exchange_weak(trueval, false, std::memory_order_acquire)) {
			switch (comm->id) {
			case SMDataID::IP :
				resolveIP(); break;

			case SMDataID::EXIT :
				return false;
			}
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	} while (1);
}

void setup_and_run() {
	CString lastError;

	// TODO: Probably less rights are sufficient
	HANDLE phandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, comm->pid);
	// we cannot validate this handle, hence pass it
	// to the resolver and handle errors there

	const char * path = "C:\\Users\\z003xucc\\.nuget\\packages\\runtime.win-x64.microsoft.netcore.app\\2.0.0\\runtimes\\win-x64\\native\\mscordaccore.dll";

	bool success = resolver.InitSymbolResolver(phandle, path, lastError);
	if (!success) {
		std::cerr << "Failure" << std::endl;
		std::cerr << lastError.GetString() << std::endl;
		return;
	}

	bool active = true;
	while (active) {
		active = process_msg();
	}
}

int main(int argc, char** argv) {
	std::cout << "Starting Managed Stack Resolver, waiting for Drace" << std::endl;

	/*
	* The general idea here is to wait until DRACE
	* sets the pid in the shared memory.
	* Hence, msr can be started prior to drace
	*/

	try {
		SharedMemory<SMData> shared(DRACE_SMR_NAME, true);
		comm = shared.get();
		
		// wait for pid
		do {
			bool trueval = true;
			if (comm->process.compare_exchange_weak(trueval, false, std::memory_order_acquire)) {
				std::cout << "Found PID: " << comm->pid << std::endl;
				setup_and_run();
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		} while (1);
	}
	catch (std::runtime_error e) {
		std::cerr << "Error initialising shared memory: " << e.what() << std::endl;
	}
	std::cout << "Quit Managed Stack Resolver" << std::endl;
}