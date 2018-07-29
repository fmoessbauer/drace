#pragma once

#include <string>
#include <vector>
#include <chrono>

/* Utility functions for all modules */
struct util {
	/* Returns true if a starts with prefix b */
	static bool common_prefix(const std::string& a, const std::string& b);

	static std::vector<std::string> util::split(
		const std::string & str,
		const std::string & delimiter);

	static std::string to_iso_time(std::chrono::system_clock::time_point tp);
};
