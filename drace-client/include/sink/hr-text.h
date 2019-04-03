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
		template<typename Stream = std::ofstream>
		class HRText {
		public:
			using self_t = HRText<Stream>;

		private:
			Stream & _stream;

		public:
			HRText() = delete;
			HRText(const self_t &) = delete;
			HRText(self_t &&) = default;

			explicit HRText(Stream & stream)
				: _stream(stream)
			{ }

			//self_t & operator=(const self_t & other) = delete;
			//self_t & operator=(self_t && other) = default;

			template<typename RaceEntry>
			void process_single_race(const RaceEntry & race) const {
				std::stringstream ss;
				ss << "----- DATA Race at " << std::dec << race.first << "ms runtime -----";
				std::string header = ss.str();
				auto & s = _stream;
				s << header << std::endl;
				for (int i = 0; i != 2; ++i) {
					ResolvedAccess ac = (i == 0) ? race.second.first : race.second.second;

					s << "Access " << i << " tid: " << std::dec << ac.thread_id << " ";
					s << (ac.write ? "write" : "read") << " to/from " << (void*)ac.accessed_memory
						<< " with size " << ac.access_size << ". Stack(Size " << ac.stack_size << ")" << std::endl;
					if (ac.onheap) {
						s << "Block begin at " << std::hex << ac.heap_block_begin << ", size " << std::dec << ac.heap_block_size << std::endl;
					}
					else {
						s << "Block not on heap (anymore)" << std::endl;
					}

					if (race.second.is_resolved) {
						// stack is stored in reverse order, hence print inverted
						int ssize = static_cast<int>(ac.resolved_stack.size());
						for (int p = 0; p < ssize; ++p) {
							s << "#" << p << " " << ac.resolved_stack[ssize - 1 - p].get_pretty();
						}
					}
					else {
						s << "-> (unresolved stack size: " << ac.stack_size << ")" << std::endl;
					}
				}
				s << std::string(header.length(), '-') << std::endl;
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
