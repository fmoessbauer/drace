#include "MStackResolver.h"

#include <iostream>

int main(int argc, char** argv) {
	std::cout << "Starting Managed Stack Resolver, enter PID" << std::endl;

	DWORD pid = 0;
	std::cin >> pid;

	CString lastError;
	HANDLE phandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	// we cannot validate this handle, hence pass it
	// to the resolver and handle errors there

	ManagedResolver resolver;

	bool success = resolver.InitSymbolResolver(phandle, lastError);
	if (!success) {
		std::cerr << "Failure" << std::endl;
		std::cerr << lastError.GetString() << std::endl;
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