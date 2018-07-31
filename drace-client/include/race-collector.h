#pragma once

#include "globals.h"
#include "symbols.h"
#include "sink/hr-text.h"

#include <detector_if.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <chrono>

#include <dr_api.h>

class ResolvedAccess : public detector::AccessEntry {
public:
	std::vector<SymbolLocation> resolved_stack;

	ResolvedAccess(const detector::AccessEntry & e)
		: detector::AccessEntry(e) { }
};

class DecoratedRace {
public:
	ResolvedAccess first;
	ResolvedAccess second;
	bool           is_resolved{ false };

	DecoratedRace(const detector::Race & r)
		: first(r.first), second(r.second) { }

	DecoratedRace(ResolvedAccess && a, ResolvedAccess && b)
		: first(a), second(b), is_resolved(true) { }
};

class RaceCollector {
public:
	using RaceEntryT = std::pair<unsigned long long, DecoratedRace>;
	using RaceCollectionT = std::vector<RaceEntryT>;
private:
	using entry_t = RaceEntryT;
	using clock_t = std::chrono::high_resolution_clock;
	using tp_t = decltype(clock_t::now());

	RaceCollectionT _races;
	// TODO: histogram

	bool   _delayed_lookup{ false };
	std::shared_ptr<Symbols> _syms;
	tp_t   _start_time;

	sink::HRText<decltype(std::cout)> _console;

public:
	RaceCollector(
		bool delayed_lookup,
		const std::shared_ptr<Symbols> & symbols)
		: _delayed_lookup(delayed_lookup),
		  _syms(symbols),
		  _start_time(clock_t::now()),
		  _console(std::cout)
	{
		_races.reserve(1000);
	}

	/* Adds a race and updates histogram */
	void add_race(const detector::Race * r) {
		auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - _start_time);

		if (!_delayed_lookup) {
			DecoratedRace dr(
			std::move(resolve_symbols(r->first)),
			std::move(resolve_symbols(r->second)));
			_races.emplace_back(ttr.count(), dr);
		}
		else {
			_races.emplace_back(ttr.count(), *r);
		}
		print_last_race();
	}

	/* Takes a detector Access Entry, resolves symbols and converts it to a ResolvedAccess */
	ResolvedAccess resolve_symbols(const detector::AccessEntry & e) const {
		ResolvedAccess ra(e);
		for (auto & f : e.stack_trace) {
			ra.resolved_stack.emplace_back(_syms->get_symbol_info((app_pc)f));
		}
		return ra;
	}

	/* Resolves all unresolved race entries */
	void resolve_all() {
		for (auto & r : _races) {
			if (!r.second.is_resolved) {
				r.second.first = std::move(resolve_symbols(r.second.first));
				r.second.second = std::move(resolve_symbols(r.second.second));
			}
		}
	}

	inline void print_last_race() const {
		DR_ASSERT(!dr_using_app_state(dr_get_current_drcontext()));
		_console.process_single_race(_races.back());
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
				auto syms = lookup_syms((app_pc)entry);
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

	const RaceCollectionT & get_races() const {
		return _races;
	}

	/* Write in Human Readable Format */
	template<typename Stream>
	void write_hr(Stream & s) const {
		for (const auto & r : _races) {
			//get_race_info(r, s);
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
