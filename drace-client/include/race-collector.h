#pragma once

#include "globals.h"
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
public:
	using LookupFuncT = std::function<SymbolLocation(void*)>;
private:
	using entry_t = std::pair<unsigned long long, detector::Race>;
	using clock_t = std::chrono::high_resolution_clock;
	using tp_t = decltype(clock_t::now());

	std::vector<entry_t> _races;
	// TODO: histogram

	bool   _delayed_lookup{ false };
	LookupFuncT _lookup_function{nullptr};
	tp_t   _start_time;

public:
	RaceCollector(
		bool delayed_lookup,
		LookupFuncT lookup_clb)
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
		DR_ASSERT(!dr_using_app_state(dr_get_current_drcontext()));
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

			for (auto entry : ac.stack_trace) {
				s << "-> " << lookup_syms((void*)entry, force_lookup).get_pretty() << std::endl;
			}
		}
		s << std::string(header.length(), '-') << std::endl;
	}

	SymbolLocation lookup_syms(void* pc, bool force_lookup = false) const {
		if (_lookup_function != nullptr && (!_delayed_lookup || force_lookup)) {
			// Type of stack_demangler: (void*) -> symbol_location_t
			SymbolLocation csloc = _lookup_function(pc);
			return csloc;
		}
		return SymbolLocation();
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
		for (unsigned i = 0; i < _races.size() * 2; ++i) {
			auto & race = (i % 2 == 0) ? _races[i / 2].second.first : _races[i / 2].second.second;
			s << "  <error>\n"
				<< "    <unique>0x" << std::hex << i << "</unique>\n"
				<< "    <tid>" << std::dec << race.thread_id << "</tid>\n"
				<< "    <kind>Race</kind>\n"
				<< "    <stack>\n";
			for (auto entry : race.stack_trace) {
				auto syms = lookup_syms((void*)entry, true);
				s <<   "      <frame>\n"
					<< "        <ip>0x" << std::hex << (uint64_t)syms.pc << "</ip>\n"
					<< "        <obj>" << syms.mod_name << "</obj>\n"
					<< "        <fn>" << syms.sym_name << "</fn>\n"
					<< "        <dir>" << syms.file.substr(0, syms.file.find_last_of("/\\")) << "</dir>\n"
					<< "        <file>" << syms.file.substr(syms.file.find_last_of("/\\") + 1) << "</file>\n"
					<< "        <line>" << std::dec << syms.line << "</line>\n"
					<< "      </frame>\n";
			}
			s <<   "    </stack>\n"
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
		return static_cast<unsigned long>(_races.size());
	}

	template<typename Stream>
	void print_summary(Stream & s) const {
		s << "Found " << _races.size() << " possible data-races" << std::endl;
	}
};

/* This function provides a callback to the RaceCollector::add_race 
*  on the singleton object. As we have to pass this callback to
*  as a function pointer to c, we cannot use std::bind
*/
static void race_collector_add_race(const detector::Race * r) {
	race_collector->add_race(r);
}
