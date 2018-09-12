#pragma once

#include <atomic>

#define DRACE_SMR_NAME "drace-msr"
#define DRACE_SMR_MAXLEN 256

enum class SMDataID : uint8_t {
	PID,
	IP,
	EXIT
};

struct IpEntry {
	uint64_t  ip;
	char      symbol[DRACE_SMR_MAXLEN];
};

struct SMData {
	// Remote sets this after new data is copied
	std::atomic<bool> process{ false };

	// MSR sets this after results are available
	std::atomic<bool> ready{ false };

	// TODO: Introduce Message IDs
	SMDataID id;

	// PID of analyzed application
	int pid;

	// Data
	IpEntry data;
};
