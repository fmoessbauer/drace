#pragma once

#include "drace-client.h"

#include <detector_if.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>

class RaceCollector {
private:
	using entry_t = std::pair<unsigned long long, detector::Race>;
	using clock_t = std::chrono::high_resolution_clock;
	using tp_t = decltype(clock_t::now());

	std::vector<entry_t> _races;
	// TODO: histogram

	bool   _delayed_lookup{ false };
	void  *_lookup_function{ nullptr };
	tp_t   _start_time;

public:
	RaceCollector(
		bool delayed_lookup,
		void* lookup_clb)
		: _delayed_lookup(delayed_lookup),
		  _lookup_function(lookup_clb),
		  _start_time(clock_t::now())
	{
		
	}

	/* Adds a race and updates histogram */
	void add_race(const detector::Race * r) {
		auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - _start_time);
		_races.emplace_back(ttr.count(), *r);
		print_last_race();
	}

	inline void print_last_race() const {
		get_race_info(_races.back(), std::cout);
	}

	template<typename Stream>
	void get_race_info(const entry_t & race, Stream & s, bool force_lookup = false) const {
		std::stringstream ss;
		ss << "----- DATA Race at " << std::dec << race.first << "ms runtime -----";
		std::string header = ss.str();
		s << header << std::endl;
		for (int i = 0; i != 2; ++i) {
			detector::AccessEntry ac;
			if (i == 0) {
				ac = race.second.first;
				s << "Access 1 tid: " << ac.thread_id << " ";
			}
			else {
				ac = race.second.second;
				s << "Access 2 tid: " << ac.thread_id << " ";
			}
			s << (ac.write ? "write" : "read") << " to/from " << ac.accessed_memory
			  << " with size " << ac.access_size << ". Stack(Size " << ac.stack_trace.size() << ")"
			  << "Type: " << std::dec << ac.access_type << " :" << ::std::endl;
			if (ac.onheap) {
				s << "Block begin at " << std::hex << ac.heap_block_begin << ", size " << ac.heap_block_size << std::endl;
			}
			else {
				s << "Block not on heap (anymore)" << std::endl;
			}
			//mxspin.unlock();

			// demagle stack
			if (_lookup_function != nullptr && (!_delayed_lookup || force_lookup)) {
				// Type of stack_demangler: (void*) -> std::string
				s << ((std::string(*)(void*))_lookup_function)(ac.stack_trace.data()) << std::endl;
			}
		}
		s << std::string(header.length(), '-') << std::endl;
	}

	void lookup_syms() const {
		
	}

	/* Write in XML Format */
	template<typename Stream>
	void write_xml(Stream & s) const {
			
	}

	/* Write in Human Readable Format */
	template<typename Stream>
	void write_hr(Stream & s) const {
		for (const auto & r : _races) {
			get_race_info(r, s, true);
		}
	}

	unsigned long num_races() const {
		return _races.size();
	}

	template<typename Stream>
	void print_summary(Stream & s) const {
		s << "Found " << _races.size() << " possible data-races" << std::endl;
	}
};

extern std::unique_ptr<RaceCollector> race_collector;

static void race_collector_add_race(const detector::Race * r) {
	race_collector->add_race(r);
}
