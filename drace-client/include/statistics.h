#pragma once

#include <vector>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>

#include <dr_api.h>

class Statistics {
public:
	using ms_t = std::chrono::milliseconds;

	std::vector<thread_id_t> thread_ids;
	unsigned long mutex_ops{ 0 };
	unsigned long flushes{ 0 };
	unsigned long flush_events{ 0 };
	unsigned long external_flushes{ 0 };
	ms_t time_in_flushes{ 0 };
	unsigned long module_loads{ 0 };
	ms_t module_load_duration{ 0 };
	unsigned long long num_refs{ 0 };

public:

	Statistics() = default;

	explicit Statistics(thread_id_t tid) {
		thread_ids.push_back(tid);
	}

	template<typename Stream>
	void print_summary(Stream & s = std::cout) {
		s << std::string(20, '-') << std::endl
			<< "Cumulative Stats for:" << std::endl
			<< "threads:\t\t" << std::dec;
		std::copy(thread_ids.begin(), thread_ids.end(),
			std::ostream_iterator<thread_id_t>(s, ","));
		s << std::endl;
		s << "mutex_ops:\t\t" << std::dec << mutex_ops << std::endl
			<< "all-flushes:\t\t" << std::dec << flush_events << std::endl
			<< "flushes:\t\t" << std::dec << flushes << std::endl;
		if (flushes > 0) {
			s << "avg. buffer size:\t" << std::dec << (num_refs / flushes) << std::endl;
		}
		s << "e-flushes:\t\t" << std::dec << external_flushes << std::endl
			<< "flush-time (total):\t" << std::dec << time_in_flushes.count() << "ms" << std::endl
			<< "memory-refs:\t\t" << std::dec << num_refs << std::endl
			<< "module loads:\t\t" << std::dec << module_loads << std::endl
			<< "mod. load time(total):\t" << std::dec << module_load_duration.count() << "ms" << std::endl
			<< std::string(20, '-') << std::endl;
	}

	inline Statistics & operator|= (const Statistics & other) {
		thread_ids.insert(thread_ids.end(), other.thread_ids.begin(), other.thread_ids.end());
		mutex_ops += other.mutex_ops;
		flushes += other.flushes;
		flush_events += other.flush_events;
		external_flushes += other.external_flushes;
		time_in_flushes += other.time_in_flushes;
		module_loads += other.module_loads;
		module_load_duration += other.module_load_duration;
		num_refs += other.num_refs;
		return *this;
	}
};