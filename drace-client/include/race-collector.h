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
#include <drmgr.h>
#include <detector/Detector.h>

#include "race-filter.h"
#include "race/DecoratedRace.h"
#include "sink/sink.h"
#include "statistics.h"
#include "ipc/DrLock.h"

#include <shared_mutex>
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
        DrLock _races_lock;
        unsigned long _race_count{ 0 };

        bool                     _delayed_lookup{ false };
        std::shared_ptr<Symbols> _syms;
        tp_t                     _start_time;
        std::set<uint64_t>       _racy_stacks;

        std::vector<std::shared_ptr<sink::Sink>> _sinks;
        
        std::shared_ptr<RaceFilter> _filter;

    public:
        RaceCollector(
            bool delayed_lookup,
            const std::shared_ptr<Symbols> & symbols,
            std::shared_ptr<RaceFilter> filter)
            : _delayed_lookup(delayed_lookup),
            _syms(symbols),
            _start_time(clock_t::now()),
            _filter(filter)
            
        {
            _races.reserve(1);
        }

        ~RaceCollector() {}

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
        void add_race(const Detector::Race * r) {
            if (num_races() > MAX)
                return;

            auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - _start_time);

            std::lock_guard<DrLock> lock(_races_lock);

            if (!filter_duplicates(r) && !filter_excluded(r)) {

                DR_ASSERT(r->first.stack_size > 0);
                DR_ASSERT(r->second.stack_size > 0);

                _races.emplace_back(*r, ttr);
                if (!_delayed_lookup) {
                    resolve_race(_races.back());
                    if(_filter->check_suppress(_races.back())) { return; }
                }
                forward_race(_races.back());
                // destroy race in streaming mode
                if (!_delayed_lookup) {
                    _races.pop_back();
                }
                _race_count += 1;
            }
        }

        /**
        * Resolves all unresolved race entries
        *
        * \note threadsafe
        */
        void resolve_all() {
            std::lock_guard<DrLock> lock(_races_lock);

            for (auto r = _races.begin(); r != _races.end(); ) {
                resolve_race(*r);
                if(_filter->check_suppress(*r)){
                    _race_count -= 1;
                    r = _races.erase(r); //erase returns iterator to element after the erased one
                } 
                else{
                    r++;
                }
            }
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
        * Filter false-positive data-races
        * \return true if race is suppressed
        */
        bool filter_excluded(const Detector::Race * r) {
            // PC is null
            if (r->first.stack_trace[r->first.stack_size - 1] == 0x0)
                return true;
            if (r->second.stack_trace[r->second.stack_size - 1] == 0x0)
                return true;

            return false;
        }

        /**
        * suppress this race if similar race is already reported
        * \return true if race is suppressed
        * \note   not-threadsafe
        */
        bool filter_duplicates(const Detector::Race * r) {
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
    static void race_collector_add_race(const Detector::Race * r) {
        race_collector->add_race(r);
        // for benchmarking and testing
        if (params.break_on_race) {
            
            void * drcontext = dr_get_current_drcontext();
            per_thread_t * data = (per_thread_t*) drmgr_get_tls_field(drcontext, tls_idx);
            data->stats->print_summary(drace::log_target);
            dr_flush_file(drace::log_target);
            dr_abort();
        }
    }

} // namespace drace
