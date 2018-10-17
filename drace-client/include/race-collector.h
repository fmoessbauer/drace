#pragma once

#include "globals.h"
#include "symbols.h"
#include "sink/hr-text.h"

#include "MSR.h"

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
		: detector::AccessEntry(e)
	{
		std::copy(e.stack_trace, e.stack_trace + e.stack_size, this->stack_trace);
	}
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

	/* Maximum number of races to collect */
	static constexpr int MAX = 1000;
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

	void *_race_mx;

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
		_race_mx = dr_mutex_create();
	}

	~RaceCollector() {
		dr_mutex_destroy(_race_mx);
	}

	/* Adds a race and updates histogram
	* TODO: The locking has to be removed completely as this callback 
	* heavily interferes with application locks. Use lockfree queue for 
	* storing the races
	*/
	void add_race(const detector::Race * r) {
		if (num_races() > MAX)
			return;

		auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - _start_time);

		if (!_delayed_lookup) {
			DecoratedRace dr(
			std::move(resolve_symbols(r->first)),
			std::move(resolve_symbols(r->second)));

			dr_mutex_lock(_race_mx);
			_races.emplace_back(ttr.count(), dr);
			dr_mutex_unlock(_race_mx);
		}
		else {
			dr_mutex_lock(_race_mx);
			_races.emplace_back(ttr.count(), *r);
			dr_mutex_unlock(_race_mx);
		}
		print_last_race();
	}

	/* Takes a detector Access Entry, resolves symbols and converts it to a ResolvedAccess */
	ResolvedAccess resolve_symbols(const detector::AccessEntry & e) const {
		ResolvedAccess ra(e);
		for (unsigned i = 0; i < e.stack_size; ++i) {
			ra.resolved_stack.emplace_back(_syms->get_symbol_info((app_pc)e.stack_trace[i]));
		}

		void* drcontext = dr_get_current_drcontext();
		dr_mcontext_t mc;
		mc.size = sizeof(dr_mcontext_t);
		mc.flags = DR_MC_ALL;
		dr_get_mcontext(drcontext, &mc);

		// TODO: Validate external callstacks
		//if(shmdriver)
		//	MSR::getCurrentStack(e.thread_id, (void*)mc.xbp, (void*)mc.xsp, (void*)e.stack_trace[e.stack_size-1]);

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

	const RaceCollectionT & get_races() const {
		return _races;
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
