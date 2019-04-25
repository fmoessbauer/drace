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
#include <atomic>

namespace drace {
    /**
    * \brief collects all detected races and manages symbol resolving
    *
    * Note for developers: Be very cautious with concurrency in this class.
    * Locking has to be avoided if possible, as this might interfere with
    * locks held by the application.
    */
    class RaceCollector {
    public:
        /** Maximum number of races to collect in delayed mode */
        static constexpr int MAX = 200;
    private:
        using RaceCollectionT = std::vector<race::DecoratedRace>;
        using entry_t = race::DecoratedRace;
        using clock_t = std::chrono::high_resolution_clock;
        using tp_t = decltype(clock_t::now());

        RaceCollectionT _races;
        /// guards all accesses to the _races container
        void * _races_lock;
        unsigned long _race_count{ 0 };

        bool                     _delayed_lookup{ false };
        std::shared_ptr<Symbols> _syms;
        tp_t                     _start_time;
        std::set<uint64_t>       _racy_stacks;

        std::vector<std::shared_ptr<sink::Sink>> _sinks;

    public:
        RaceCollector(
            bool delayed_lookup,
            const std::shared_ptr<Symbols> & symbols)
            : _delayed_lookup(delayed_lookup),
            _syms(symbols),
            _start_time(clock_t::now())
        {
            _races.reserve(1);
            _races_lock = dr_mutex_create();
        }

        ~RaceCollector() {
            dr_mutex_destroy(_races_lock);
        }

        /**
        * register a sink that is notified on each race
        * \note not-threadsafe
        */
        void register_sink(const std::shared_ptr<sink::Sink> & sink) {
            _sinks.push_back(sink);
        }

        /** Adds a race and updates histogram
        *
        * \note threadsafe
        */
        void add_race(const detector::Race * r) {
            if (num_races() > MAX)
                return;

            auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - _start_time);

            dr_mutex_lock(_races_lock);
            if (!filter_duplicates(r)) {
                _races.emplace_back(*r, ttr);
                if (!_delayed_lookup) {
                    resolve_race(_races.back());
                }
                forward_race(_races.back());
                // destroy race in streaming mode
                if (!_delayed_lookup) {
                    _races.pop_back();
                }
                _race_count += 1;
            }
            dr_mutex_unlock(_races_lock);
        }

        /**
        * Resolves all unresolved race entries
        *
        * \note threadsafe
        */
        void resolve_all() {
            dr_mutex_lock(_races_lock);
            for (auto & r : _races) {
                resolve_race(r);
            }
            dr_mutex_unlock(_races_lock);
        }

        /**
        * In delayed mode, return data-races.
        * Otherwise return empty container
        *
        * \note not-threadsafe
        */
        const std::vector<race::DecoratedRace> & get_races() const {
            return _races;
        }

        /**
        * return the number of observed races
        */
        unsigned long num_races() const {
            return _race_count;
        }

    private:
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

        /** Takes a detector Access Entry, resolves symbols and converts it to a ResolvedAccess */
        void resolve_symbols(race::ResolvedAccess  & ra) {
            for (unsigned i = 0; i < ra.stack_size; ++i) {
                ra.resolved_stack.emplace_back(_syms->get_symbol_info((app_pc)ra.stack_trace[i]));
            }

#if 0
            // TODO: possibly use this information to refine callstack
            void* drcontext = dr_get_current_drcontext();
            dr_mcontext_t mc;
            mc.size = sizeof(dr_mcontext_t);
            mc.flags = DR_MC_ALL;
            dr_get_mcontext(drcontext, &mc);
#endif
        }

        /** resolve a single race */
        void resolve_race(race::DecoratedRace & race) {
            if (!race.is_resolved) {
                resolve_symbols(race.first);
                resolve_symbols(race.second);
            }
            race.is_resolved = true;
        }

        /**
        * forward a single race to the sinks
        *
        * \note not-threadsafe
        */
        inline void forward_race(const race::DecoratedRace & race) const {
            for (const auto & s : _sinks) {
                s->process_single_race(race);
            }
        }
    };

    /**
    * This function provides a callback to the RaceCollector::add_race
    * on the singleton object. As we have to pass this callback to
    * as a function pointer to c, we cannot use std::bind
    */
    static void race_collector_add_race(const detector::Race * r) {
        race_collector->add_race(r);
        // for benchmarking and testing
        if (params.break_on_race) {
            dr_abort();
        }
    }

} // namespace drace
