#include "ManagedResolver.h"
#include "ipc/msr-driver.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

ManagedResolver resolver;
std::shared_ptr<MsrDriver<false, false>> msrdriver;
bool keep_running{ true };

void attachProcess() {
	CString lastError;

	// TODO: Probably less rights are sufficient
	int pid = msrdriver->get<int>();
	HANDLE phandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	// we cannot validate this handle, hence pass it
	// to the resolver and handle errors there

	const char * path = "C:\\Users\\z003xucc\\.nuget\\packages\\runtime.win-x64.microsoft.netcore.app\\2.0.0\\runtimes\\win-x64\\native\\mscordaccore.dll";

	bool success = resolver.InitSymbolResolver(phandle, path, lastError);
	if (!success) {
		std::cerr << "Failure" << std::endl;
		std::cerr << lastError.GetString() << std::endl;
		return;
	}
	std::cout << "--- Attached to " << pid << " ---" << std::endl;
	msrdriver->state(SMDataID::ATTACHED);
	msrdriver->commit();
}

void detachProcess() {
	std::cout << "--- Detach MSR ---" << std::endl;
	resolver.Close();
	msrdriver->state(SMDataID::CONNECT);
	msrdriver->commit();
}

void resolveIP() {
	CString buffer;
	void* ip = msrdriver->get<void*>();
	std::cout << "Resolve IP: " << ip << std::endl;

	SymbolInfo & sym = msrdriver->get<SymbolInfo>();

	// Get Module Name
	resolver.GetModuleName(ip, buffer);
	strncpy(sym.module, buffer.GetBuffer(128), 128);

	// Get Function Name
	buffer = "";
	resolver.GetMethodName(ip, buffer);
	strncpy(sym.function, buffer.GetBuffer(128), 128);

	// Get Line Info
	buffer = "";
	resolver.GetFileLineInfo(ip, buffer);
	strncpy(sym.path, buffer.GetBuffer(128), 128);

	msrdriver->commit();
}

void process_msg() {
	// wait for incoming requests
	do {
		if (msrdriver->wait_receive()) {
			switch (msrdriver->id()) {
			case SMDataID::PID :
				attachProcess(); break;
			case SMDataID::IP :
				resolveIP(); break;
			case SMDataID::EXIT :
				detachProcess(); break;
			default:
				std::cerr << "protocol error" << std::endl; keep_running = false;
			}
		}
	} while (keep_running);
}

BOOL CtrlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		keep_running = false;
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
		msrdriver = std::make_unique<MsrDriver<false, false>>(DRACE_SMR_NAME, true);
		process_msg();
	}
	catch (std::runtime_error e) {
		std::cerr << "Error initialising shared memory: " << e.what() << std::endl;
	}
	std::cout << "Quit Managed Stack Resolver" << std::endl;
}