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

	static std::string dir_from_path(const std::string & fullpath);

	static std::string basename(const std::string & fullpath);

	static std::string to_iso_time(std::chrono::system_clock::time_point tp);

	static std::string instr_flags_to_str(uint8_t flags);
};

// Logging

#define LOG_HELPER(tid, level, msg, ...) do {dr_printf("[drace][%.5i][%s] " msg "\n", tid, level, __VA_ARGS__);} while (0)

#define LOG(tid, level, msg, ...) do {\
	if(tid == 0){LOG_HELPER(dr_get_thread_id(dr_get_current_drcontext()), level, msg, __VA_ARGS__);}\
    else {LOG_HELPER(tid, level, msg, __VA_ARGS__);}} while (0)


#if LOGLEVEL > 3
#define LOG_TRACE(tid, msg, ...)  LOG(tid, " trace", msg, __VA_ARGS__)
#else
#define LOG_TRACE(tid, msg, ...)
#endif

#if LOGLEVEL > 2
#define LOG_NOTICE(tid, msg, ...) LOG(tid, "notice", msg, __VA_ARGS__)
#else
#define LOG_NOTICE(tid, msg, ...)
#endif

#if LOGLEVEL > 1
#define LOG_INFO(tid, msg, ...)   LOG(tid, "  info", msg, __VA_ARGS__)
#else
#define LOG_INFO(tid, msg, ...)
#endif

#if LOGLEVEL > 0
#define LOG_WARN(tid, msg, ...)   LOG(tid, "  warn", msg, __VA_ARGS__)
#else
#define LOG_WARN(tid, msg, ...)
#endif

#define LOG_ERROR(tid, msg, ...)  LOG(tid, " error", msg, __VA_ARGS__)
