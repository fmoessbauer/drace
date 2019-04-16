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

#include "../race-collector.h"

#include <sstream>

namespace drace {
	/// data-race exporter
	namespace sink {
		/** A Race exporter which creates human readable output */
		class HRText {
		public:
			using self_t = HRText;

		private:
			FILE * _target;

		public:
			HRText() = delete;
			HRText(const self_t &) = delete;
			HRText(self_t &&) = default;

			explicit HRText(FILE * target)
				: _target(target)
			{ }

			//self_t & operator=(const self_t & other) = delete;
			//self_t & operator=(self_t && other) = default;

			template<typename RaceEntry>
			void process_single_race(const RaceEntry & race) const {
                LOG_ERROR(0, "process single %u", _target);
				std::stringstream ss;
				ss << "----- DATA Race at " << std::dec << race.first << "ms runtime -----";
				std::string header(ss.str());
                ss.clear();

                dr_fprintf(_target, "%s\n", header.c_str());

				for (int i = 0; i != 2; ++i) {
					ResolvedAccess ac = (i == 0) ? race.second.first : race.second.second;

                    dr_fprintf(_target, "Access %i tid: %i %s to/from %p with size %i. Stack (size %i)\n",
                        i, ac.thread_id, (ac.write ? "write" : "read"), (void*)ac.accessed_memory,
                        ac.access_size, ac.stack_size);

					if (ac.onheap) {
                        dr_fprintf(_target, "Block begin at %p, size %u\n", ac.heap_block_begin, ac.heap_block_size);
			        }
					else {
                        dr_fprintf(_target, "Block not on heap (anymore)\n");
					}

					if (race.second.is_resolved) {
						// stack is stored in reverse order, hence print inverted
						int ssize = static_cast<int>(ac.resolved_stack.size());
						for (int p = 0; p < ssize; ++p) {
                            dr_fprintf(_target, "# %p %s", p, ac.resolved_stack[ssize - 1 - p].get_pretty().c_str());
						}
					}
					else {
                        dr_fprintf(_target, "-> (unresolved stack size: %u)\n", ac.stack_size);
					}
				}
				dr_fprintf(_target, "%s\n", std::string(header.length(), '-').c_str());
                dr_flush_file(_target);
            }

			template<typename RaceEntry>
			void process_all(const RaceEntry & races) {
				for (auto & r : races) {
					process_single_race(r);
				}
			}
		};

	} // namespace sink
} // namespace drace
