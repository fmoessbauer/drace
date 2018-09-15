#pragma once

#include <atomic>

#define DRACE_SMR_NAME "drace-msr"
#define DRACE_SMR_MAXLEN 1024

namespace ipc {

	enum class SMDataID : uint8_t {
		PID,
		IP,
		SYMBOL,
		CONNECT,
		ATTACHED,
		LOADSYMS,

		WAIT,
		CONFIRM,

		EXIT
	};

	struct SymbolInfo {
		char      module[128];
		char      function[128];
		char      path[128];
		char      line[64];
	};

	struct BaseInfo {
		int pid;
		char path[128];
	};

	struct SymbolRequest {
		uint64_t base;
		size_t   size;
		char     path[128];
	};

	struct SMData {
		// Message IDs
		SMDataID id;
		// Raw data buffer
		byte buffer[DRACE_SMR_MAXLEN - 16];
	};

} // namespace ipc
