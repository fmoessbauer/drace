#include "ProtocolHandler.h"

ProtocolHandler::ProtocolHandler(
	MsrDriverPtr msrdriver)
	: _msrdriver(msrdriver)
{
}

ProtocolHandler::~ProtocolHandler() {
	quit();
}

void ProtocolHandler::attachProcess() {
	CString lastError;

	// TODO: Probably less rights are sufficient
	int pid = _msrdriver->get<int>();
	HANDLE phandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	// we cannot validate this handle, hence pass it
	// to the resolver and handle errors there

	const char * path = "C:\\Users\\z003xucc\\.nuget\\packages\\runtime.win-x64.microsoft.netcore.app\\2.0.0\\runtimes\\win-x64\\native\\mscordaccore.dll";

	bool success = _resolver.InitSymbolResolver(phandle, path, lastError);
	if (!success) {
		std::cerr << "Failure" << std::endl;
		std::cerr << lastError.GetString() << std::endl;
		return;
	}
	std::cout << "--- Attached to " << pid << " ---" << std::endl;
	_msrdriver->state(SMDataID::ATTACHED);
	_msrdriver->commit();
}

void ProtocolHandler::detachProcess() {
	std::cout << "--- Detach MSR ---" << std::endl;
	_resolver.Close();
	_msrdriver->state(SMDataID::CONNECT);
	_msrdriver->commit();
}

void ProtocolHandler::resolveIP() {
	CString buffer;
	void* ip = _msrdriver->get<void*>();
	std::cout << "Resolve IP: " << ip << std::endl;

	SymbolInfo & sym = _msrdriver->get<SymbolInfo>();

	// Get Module Name
	_resolver.GetModuleName(ip, buffer);
	strncpy_s(sym.module, buffer.GetBuffer(128), 128);

	// Get Function Name
	buffer = "";
	_resolver.GetMethodName(ip, buffer);
	strncpy_s(sym.function, buffer.GetBuffer(128), 128);

	// Get Line Info
	buffer = "";
	_resolver.GetFileLineInfo(ip, buffer);
	strncpy_s(sym.path, buffer.GetBuffer(128), 128);

	_msrdriver->commit();
}

void ProtocolHandler::process_msgs() {
	// wait for incoming requests
	do {
		if (_msrdriver->wait_receive()) {
			switch (_msrdriver->id()) {
			case SMDataID::PID:
				attachProcess(); break;
			case SMDataID::IP:
				resolveIP(); break;
			case SMDataID::EXIT:
				detachProcess(); break;
			default:
				std::cerr << "protocol error" << std::endl;
				_keep_running = false;
			}
		}
	} while (_keep_running);
}

void ProtocolHandler::quit() {
	_keep_running = false;
}
