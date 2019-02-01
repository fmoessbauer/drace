#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */

#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <algorithm>

#include <dr_api.h>

#include <lcm/lossyCountingModel.hpp>

namespace drace {
	/**
	* Stores and processes per-thread and global statistics.
	* Instrumentation classes can use this information
	* to adapt to workload.
	*/
	class Statistics {
	public:
		using ms_t = std::chrono::milliseconds;
		using hist_t = std::vector<std::pair<uint64_t, size_t>>;

		std::vector<thread_id_t> thread_ids;
		unsigned long mutex_ops{ 0 };
		unsigned long flushes{ 0 };
		unsigned long flush_events{ 0 };
		unsigned long external_flushes{ 0 };
		ms_t time_in_flushes{ 0 };
		unsigned long module_loads{ 0 };
		ms_t module_load_duration{ 0 };
		uint64_t proc_refs{ 0 };
		uint64_t total_refs{ 0 };

		LossyCountingModel<uint64_t> page_hits;
		LossyCountingModel<uint64_t> pc_hits;

		hist_t freq_hits;
		hist_t freq_pc_hist;
		std::vector<uint64_t> freq_pcs;

	public:

		Statistics() = default;

		explicit Statistics(thread_id_t tid)
			: page_hits(0.01, 0.001),
			pc_hits(0.01, 0.001)
		{
			thread_ids.push_back(tid);
		}

		template<typename Stream>
		void print_summary(Stream & s = std::cout) {
			s << std::string(20, '-') << std::endl
				<< "Cumulative Stats for:" << std::endl
				<< "thread-ids:\t\t" << std::dec;
			std::copy(thread_ids.begin(), thread_ids.end(),
				std::ostream_iterator<thread_id_t>(s, ","));
			s << std::endl;
			s << "mutex_ops:\t\t" << std::dec << mutex_ops << std::endl
				<< "all-flushes:\t\t" << std::dec << flush_events << std::endl
				<< "flushes:\t\t" << std::dec << flushes << std::endl;
			if (flushes > 0) {
				s << "avg. buffer size:\t" << std::dec << (total_refs / flushes) << std::endl;
			}
			s << "e-flushes:\t\t" << std::dec << external_flushes << std::endl
				<< "flush-time (total):\t" << std::dec << time_in_flushes.count() << "ms" << std::endl
				<< "analyzed-refs:\t\t" << std::dec << proc_refs << std::endl
				<< "total-refs:\t\t" << std::dec << total_refs << std::endl
				<< "module loads:\t\t" << std::dec << module_loads << std::endl
				<< "mod. load time(total):\t" << std::dec << module_load_duration.count() << "ms" << std::endl;
			s << "top pages:\t\t";
			for (const auto & p : freq_hits) {
				s << "(" << std::hex << p.first << "," << std::dec << p.second << "),";
			}

			s << std::endl << "top pcs:\t\t";
			const auto & state = pc_hits.getState();
			for (const auto & p : freq_pc_hist) {
				double share = (static_cast<double>(p.second) / state.N)*(100.0 + state.e);
				s << "(" << std::setw(8) << std::hex << p.first << ","
					<< std::dec << std::setprecision(3) << std::setw(2) << share << "%),";
			}
			s << std::endl << std::string(20, '-') << std::endl;
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
			proc_refs += other.proc_refs;
			total_refs += other.total_refs;
			return *this;
		}
	};
} // namespace drace