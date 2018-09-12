#include "ManagedResolver.h"

#include <iostream>

int main(int argc, char** argv) {
	std::cout << "Starting Managed Stack Resolver, enter PID" << std::endl;

	DWORD pid = 0;
	std::cin >> pid;

	CString lastError;
	// TODO: Probably less rights are sufficient
	HANDLE phandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	// we cannot validate this handle, hence pass it
	// to the resolver and handle errors there

	ManagedResolver resolver;

	const char * path = "C:\\Users\\z003xucc\\.nuget\\packages\\runtime.win-x64.microsoft.netcore.app\\2.0.0\\runtimes\\win-x64\\native\\mscordaccore.dll";

	bool success = resolver.InitSymbolResolver(phandle, path, lastError);
	if (!success) {
		std::cerr << "Failure" << std::endl;
		std::cerr << lastError.GetString() << std::endl;
		return 1;
	}

	uint64_t pc;
	for (int i = 0; i < 10; ++i) {
		std::cout << "Enter PC (in hex): ";
		std::cin >> std::hex >> pc;
		CString symbol;
		resolver.GetMethodName((void*)pc, symbol);
		std::cout << "Found: " << symbol.GetString() << std::endl;
	}

	std::cout << "Quit Managed Stack Resolver" << std::endl;
}