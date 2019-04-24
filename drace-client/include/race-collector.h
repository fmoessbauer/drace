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
    * Locking has to be avoided at all cost, as this heavily interferes with
    * locks held by the application.
    */
    class RaceCollector {
    public:
        /** Maximum number of races to collect in delayed mode */
        static constexpr int MAX = 200;
    private:
        using RaceCollectionT = std::vector<std::unique_ptr<race::DecoratedRace>>;
        using entry_t = race::DecoratedRace;
        using clock_t = std::chrono::high_resolution_clock;
        using tp_t = decltype(clock_t::now());

        RaceCollectionT _races;
        std::atomic<uint_fast64_t> _race_count{ 0 };
        // TODO: histogram

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
            // we have to make sure the vector does not resize at any time
            _races.resize(MAX);
        }

        /**
        * register a sink that is notified on each race
        */
        void register_sink(const std::shared_ptr<sink::Sink> & sink) {
            _sinks.push_back(sink);
        }

        /** Adds a race and updates histogram
        *
        * \note we must NOT use any locks here, as this callback heavily interferes
        *       with application locks
        * \note threadsafe
        */
        void add_race(const detector::Race * r) {
            if (num_races() > MAX)
                return;

            auto ttr = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - _start_time);

            // TODO: as a first workaround for the locking issues here,
            //       add a skip guard (let second concurrent call just skip this)

            // TODO: this is not thread-safe, find workaround
            if (filter_duplicates(r))
                return;

            auto rptr = std::make_unique<race::DecoratedRace>(*r, ttr);
            resolve_race(*rptr);
            // TODO: this is not thread-safe, find workaround
            forward_race(*rptr);
            auto cur_cntr = _race_count.fetch_add(1, std::memory_order_relaxed);

            if (_delayed_lookup) {
                _races[cur_cntr] = std::move(rptr);
            }
        }

        /**
        * Resolves all unresolved race entries
        *
        * \note not-threadsafe
        */
        void resolve_all() {
            auto num_races = _race_count.load(std::memory_order_relaxed);
            for (int i = 0; i < num_races; ++i) {
                resolve_race(*_races[i]);
            }
        }

        /**
        * In delayed mode, return data-races.
        * Otherwise return empty container
        *
        * \note internally we make a copy of all races, hence expect overhead
        *       on each call. If possible, try to cache result
        * \note threadsafe, eventually consistent
        */
        const std::vector<race::DecoratedRace> get_races() const {
            std::vector<race::DecoratedRace> races;
            auto num_races = _race_count.load(std::memory_order_relaxed);
            races.reserve(num_races);
            for (int i = 0; i < num_races; ++i) {
                races.push_back(*_races[i]);
            }
            return races;
        }

        /**
        * return the number of observed races
        */
        unsigned long num_races() const {
            return static_cast<unsigned long>(_race_count);
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

        /** resolve a single race */
        void resolve_race(race::DecoratedRace & race) {
            if (!race.is_resolved) {
                race.first = std::move(resolve_symbols(race.first));
                race.second = std::move(resolve_symbols(race.second));
            }
        }

        /**
        * forward a single race to the sinks
        *
        * \note threadsafe (if sinks are thread-safe)
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
