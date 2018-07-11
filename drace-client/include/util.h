#pragma once

#include <string>
#include <vector>

/* Utility functions for all modules */
struct util {
	/* Returns true if a starts with prefix b */
	static bool common_prefix(const std::string& a, const std::string& b);

	static std::vector<std::string> util::split(
		const std::string & str,
		const std::string & delimiter);
};
