#pragma once

#include "drace-client.h"
#include "symbols.h"

#include <detector_if.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <chrono>

#include <dr_api.h>

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
		_races.reserve(1000);
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
			detector::AccessEntry ac = (i==0) ? race.second.first : race.second.second;

			s << "Access " << i << " tid: " << std::dec << ac.thread_id << " ";
			s << (ac.write ? "write" : "read") << " to/from " << ac.accessed_memory
			  << " with size " << ac.access_size << ". Stack(Size " << ac.stack_trace.size() << ")"
			  << "Type: " << std::dec << ac.access_type << " :" << ::std::endl;
			if (ac.onheap) {
				s << "Block begin at " << std::hex << ac.heap_block_begin << ", size " << ac.heap_block_size << std::endl;
			}
			else {
				s << "Block not on heap (anymore)" << std::endl;
			}

			s << lookup_syms((void*)(ac.stack_trace.data()), force_lookup).get_pretty() << std::endl;
		}
		s << std::string(header.length(), '-') << std::endl;
	}

	symbols::symbol_location_t lookup_syms(void* pc, bool force_lookup = false) const {
		using symbols::symbol_location_t;
		if (_lookup_function != nullptr && (!_delayed_lookup || force_lookup)) {
			// Type of stack_demangler: (void*) -> symbol_location_t
			symbol_location_t csloc = ((symbol_location_t(*)(void*))_lookup_function)(pc);
			return csloc;
		}
		return symbols::symbol_location_t();
	}

	/* Write in XML Format */
	template<typename Stream>
	void write_xml(Stream & s) const {
		// header
		s << "<?xml version=\"1.0\"?>\n"
			<< "<valgrindoutput>\n"
			<< "  <protocolversion>4</protocolversion>\n"
			<< "  <protocoltool>helgrind</protocoltool>\n"
			<< "  <preamble><line>Drace, a thread error detector</line></preamble>\n"
			<< "  <tool>drace</tool>\n\n";
		// errors
		for (unsigned i = 0; i < _races.size()*2; ++i) {
			auto & race = (i%2==0) ? _races[i/2].second.first : _races[i/2].second.second;
			auto syms = lookup_syms((void*)race.stack_trace.data(), true);
			s   << "  <error>\n"
			    << "    <unique>0x" << std::hex << i << "</unique>\n"
				<< "    <tid>" << std::dec << race.thread_id << "</tid>\n"
				<< "    <kind>Race</kind>\n"
				<< "    <stack>\n"
				<< "      <frame>\n"
				<< "        <ip>0x" << std::hex << (uint64_t)syms.pc << "</ip>\n"
				<< "        <obj>" << syms.mod_name << "</obj>\n"
				<< "        <fn>" << syms.sym_name << "</fn>\n"
				<< "        <dir>" << syms.file.substr(0, syms.file.find_last_of("/\\")) << "</dir>\n"
				<< "        <file>" << syms.file.substr(syms.file.find_last_of("/\\") + 1) << "</file>\n"
				<< "        <line>" << std::dec << syms.line << "</line>\n"
				<< "      </frame>\n"
				<< "    </stack>\n"
		        << "  </error>\n";
		}
		// footer
		s << "</valgrindoutput>\n";
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
