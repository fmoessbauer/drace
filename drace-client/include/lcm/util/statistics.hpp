#ifndef STATISTICS_H
#define STATISTICS_H

/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2017, Felix Moessbauer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
