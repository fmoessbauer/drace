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

#include "Tracker.h"

namespace drace {
	namespace module {

        /**
        * \brief Simple cache to speedup module boundary lookups
        *
        * Currently the cache holds just one entry
        */
		class Cache {
		private:
			/** keep last module here for faster lookup
			*  we use a week pointer as we do not obtain ownership */
			const Metadata*  _mod{ nullptr };
			app_pc       _start;
			app_pc       _end;

		public:
			/// Lookup last module in cache, returns (found, instrument)
			inline const Metadata* lookup(const app_pc pc) const {
				if (!_mod) return nullptr;
				if (pc >= _start && pc < _end) {
					return _mod;
				}
				return nullptr;
			}

            /// put a new module into the cache
			inline void update(const Metadata * mod) {
				_mod = mod;
				_start = _mod->info->start;
				_end = _mod->info->end;
			}
		};
	} // namespace module
} // namespace drace
