#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "race/DecoratedRace.h"
#include "sink/sink.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace drace {
	class RaceCollector {
    public:
		/** Maximum number of races to collect in delayed mode */
		static constexpr int MAX = 200;
	private:
        using RaceCollectionT = std::vector<race::DecoratedRace>;
		using entry_t         = race::DecoratedRace;
		using clock_t         = std::chrono::high_resolution_clock;
		using tp_t            = decltype(clock_t::now());

		RaceCollectionT _races;
        unsigned long   _race_count{0};
		// TODO: histogram

		bool                     _delayed_lookup{ false };
		std::shared_ptr<Symbols> _syms;
		tp_t                     _start_time;
        std::set<uint64_t>       _racy_stacks;

		std::vector<std::shared_ptr<sink::Sink>> _sinks;

		void *                   _race_mx;

	public:
		RaceCollector(
			bool delayed_lookup,
			const std::shared_ptr<Symbols> & symbols)
			: _delayed_lookup(delayed_lookup),
			_syms(symbols),
			_start_time(clock_t::now())
		{
			_races.reserve(10);
			_race_mx = dr_mutex_create();
		}

		~RaceCollector() {
			dr_mutex_destroy(_race_mx);
		}

        /**
        * register a sink that is notified on each race
        */
        void register_sink(const std::shared_ptr<sink::Sink> & sink) {
            _sinks.push_back(sink);
        }

        /**
        * suppress this race if similar race is already reported
        * \return true if race is suppressed
        * \note   not-threadsafe
        */
        bool filter_duplicates(const detector::Race * r) {
            // TODO: add more precise control over suppressions
            if (params.suppression_level == 0)
                return false;

            uint64_t hash = r->first.stack_trace[0] ^ (r->second.stack_trace[0] << 1);
            if (_racy_stacks.count(hash) == 0) {
                // suppress this race
                _racy_stacks.insert(hash);
                return false;
            }
            return true;
        }

		/** Adds a race and updates histogram
		* TODO: The locking has to be removed completely as this callback
		* heavily interferes with application locks. Use lockfree queue for
		* storing the races
        *
        * \note not-threadsafe
		*/
		void add_race(const detector::Race * r) {
			if (num_races() > MAX)
				return;

			auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - _start_time);

            if (filter_duplicates(r))
                return;

            if (!_delayed_lookup) {
                race::DecoratedRace dr(
                    std::move(resolve_symbols(r->first)),
                    std::move(resolve_symbols(r->second)),
                    ttr);

                //dr_mutex_lock(_race_mx);
                _races.emplace_back(dr);
                //dr_mutex_unlock(_race_mx);
            }
			else {
				//dr_mutex_lock(_race_mx);
				_races.emplace_back(*r, ttr); // TODO, validate ttr value
				//dr_mutex_unlock(_race_mx);
			}
            ++_race_count;
            // notify sinks about this race
			forward_last_race();
            // in non-delayed mode, we can drop the race
            if (!_delayed_lookup)
                _races.pop_back();
		}

		/** Takes a detector Access Entry, resolves symbols and converts it to a ResolvedAccess */
		race::ResolvedAccess resolve_symbols(const detector::AccessEntry & e) const {
			race::ResolvedAccess ra(e);
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

		/** Resolves all unresolved race entries */
		void resolve_all() {
			for (auto & r : _races) {
				if (!r.is_resolved) {
					r.first = std::move(resolve_symbols(r.first));
					r.second = std::move(resolve_symbols(r.second));
				}
			}
		}

        /**
        * forward last observed race to sinks
        */
        inline void forward_last_race() const {
            for (const auto & s : _sinks) {
                s->process_single_race(_races.back());
            }
        }

        /**
        * In delayed mode, return data-races.
        * Otherwise return empty container
        *
        * \note not threadsafe.
        *       If a new race arrives during inspection of this container,
        *       the iterators might get invalidated
        */
		const RaceCollectionT & get_races() const {
            return _races;
		}

        /**
        * return the number of observed races
        */
		unsigned long num_races() const {
			return static_cast<unsigned long>(_race_count);
		}
	};

	/** This function provides a callback to the RaceCollector::add_race
	*  on the singleton object. As we have to pass this callback to
	*  as a function pointer to c, we cannot use std::bind
	*/
	static void race_collector_add_race(const detector::Race * r) {
		race_collector->add_race(r);
		// for benchmarking and testing
		if (params.break_on_race) {
			dr_abort();
		}
	}

} // namespace drace
