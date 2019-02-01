#pragma once
/*
 * DRace, a dynamic data race detector
 *
 * Copyright (c) Siemens AG, 2018
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * This work is licensed under the terms of the MIT license.  See
 * the LICENSE file in the top-level directory.
 */

#include "Tracker.h"

namespace drace {
	namespace module {
		class Cache {
		private:
			/** keep last module here for faster lookup
			*  we use a week pointer as we do not obtain ownership */
			Metadata*  _mod{ nullptr };
			app_pc       _start;
			app_pc       _end;

		public:
			/** Lookup last module in cache, returns (found, instrument)*/
			inline Metadata* lookup(app_pc pc) {
				if (!_mod) return nullptr;
				if (pc >= _start && pc < _end) {
					return _mod;
				}
				return nullptr;
			}
			inline void update(Metadata * mod) {
				_mod = mod;
				_start = _mod->info->start;
				_end = _mod->info->end;
			}
		};
	} // namespace module
} // namespace drace
