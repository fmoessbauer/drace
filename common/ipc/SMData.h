#pragma once

#define DRACE_SMR_NAME "drace-msr"
#define DRACE_SMR_MAXLEN 1024

namespace ipc {

	/* Protocol Message IDs */
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

	/* Resolved Symbol Information */
	struct SymbolInfo {
		char      module[128];
		char      function[128];
		char      path[128];
		char      line[64];
	};

	/* Basic Information for attaching */
	struct BaseInfo {
		// process id
		int pid;
		// Path to *clr.dll
		char path[128];
	};

	/* Request symbols for this module */
	struct SymbolRequest {
		uint64_t base;
		size_t   size;
		char     path[128];
	};

	struct SMData {
		static constexpr unsigned BUFFER_SIZE = DRACE_SMR_MAXLEN - 16;

		// Message IDs
		SMDataID id;
		// Raw data buffer
		byte buffer[BUFFER_SIZE];
	};

} // namespace ipc
