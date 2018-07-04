#pragma once

#include <string>

/* Utility functions for all modules */
struct util {
	/* Returns true if a starts with prefix b */
	static bool common_prefix(const std::string& a, const std::string& b);
};
