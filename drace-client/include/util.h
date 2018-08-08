#pragma once

#include <string>
#include <vector>
#include <chrono>

// Forward decl to avoid including globals.h
enum INSTR_FLAGS : uint8_t;

/* Utility functions for all modules */
struct util {
	/* Returns true if a starts with prefix b */
	static bool common_prefix(const std::string& a, const std::string& b);

	static std::vector<std::string> util::split(
		const std::string & str,
		const std::string & delimiter);

	static std::string to_iso_time(std::chrono::system_clock::time_point tp);

	static std::string instr_flags_to_str(INSTR_FLAGS flags);
};
