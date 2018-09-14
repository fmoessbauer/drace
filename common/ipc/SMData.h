#pragma once

#include <atomic>

#define DRACE_SMR_NAME "drace-msr"
#define DRACE_SMR_MAXLEN 512

enum class SMDataID : uint8_t {
	PID,
	IP,
	CONNECT,
	ATTACHED,
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

struct SMData {
	// Message IDs
	SMDataID id;

	byte buffer[DRACE_SMR_MAXLEN - 16];
};
