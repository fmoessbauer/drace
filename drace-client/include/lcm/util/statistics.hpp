#ifndef STATISTICS_H
#define STATISTICS_H

/**
* MIT License
*
* Copyright (c) Felix Moessbauer, 2018
*
* Authors:
*   Felix Moessbauer <felix.moessbauer@siemens.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include <chrono>
#include <iostream>

namespace drace {

	/**
	 * print Lossy Counting Model State
	 */
	template<typename State>
	void printState(const State & lcm_state) {
		std::cout << "LCM Model State: "
			<< "f:" << lcm_state.f << ", "
			<< "e:" << lcm_state.e << ", "
			<< "w:" << lcm_state.w << ", "
			<< "N:" << lcm_state.N << std::endl;
	}

	/**
	 * print performance data
	 */
	template<
		typename State,
		typename Duration>
		void printPerformance(
			const State & lcm_state,
			const int   & elem_size,
			const Duration & dur) {
		using ms = std::chrono::milliseconds;
		constexpr int MEGA = 1024 * 1024;

		auto duration = std::chrono::duration_cast<ms>(dur);
		long long processed_bytes = (lcm_state.N * elem_size);
		long long bytes_per_ms = processed_bytes / duration.count();
		long long elems_per_ms = lcm_state.N / duration.count();
		double mb_per_s = static_cast<double>(bytes_per_ms) / 1024.0;
		double elems_per_s = static_cast<double>(elems_per_ms) / 1024.0;

		std::cout << "--- Performance ---" << std::endl
			<< "Processed MB: " << (processed_bytes) / (MEGA) << std::endl
			<< "Time in ms:   " << duration.count() << std::endl
			<< "MB/s:         " << mb_per_s << std::endl
			<< "MElem/s:      " << elems_per_s << std::endl
			<< "--- ----------- ---" << std::endl;
	}
} // namespace drace
#endif
