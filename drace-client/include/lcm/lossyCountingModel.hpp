#ifndef LOSSY_COUNTING_MODEL_H
#define LOSSY_COUNTING_MODEL_H

/*
 * https://github.com/fmoessbauer/LossyCountingModel
 */
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

// on windows we should use optimized allocator,
// but that is not possible due to DR
#ifdef DRACE_OPT_HASHMAP
#include <sparsepp/spp.h>
#else
#include <unordered_map>
#endif

namespace drace {

/**
 * Lossy Counting Model implemenation for processing a
 * data stream according to the following constraints:
 * - Single Pass
 * - Limited memory
 * - Volume of data in real-time
 *
 * This is a slightly modified version of
 * https://github.com/fmoessbauer/LossyCountingModel to make it work with
 * DynamoRio
 */
template <typename T>
class LossyCountingModel {
 public:
  /**
   * holds a LCM Model state
   */
  struct State {
    double f;              /// frequency
    double e;              /// error
    unsigned long w;       /// window size
    unsigned long long N;  /// stream length
  };

 private:
#ifdef DRACE_OPT_HASHMAP
  using histogram_type = typename spp::sparse_hash_map<T, size_t>;
#else
  using histogram_type = typename std::unordered_map<T, size_t>;
#endif

  double _frequency;
  double _error;
  unsigned long _window_size;
  unsigned long long _total_processed_elements = 0;
  histogram_type _histogram;

 public:
  /**
   * Setup a Lossy Counting Model with sampling
   * frequency and error.
   */
  LossyCountingModel(double frequency, double error,
                     long estimated_length = 1e5) noexcept
      : _frequency(frequency),
        _error(error),
        _window_size(static_cast<unsigned long>(std::round(1.0 / error))) {
    // int estimated_size = static_cast<int>((1.0 / _error) * std::log2(
    //                        _error * static_cast<double>(estimated_length)));
    //_histogram.reserve(estimated_size);
  }

  /**
   * process a single item of the datastream
   * \return true if the sampling window was processed after this item,
   *         false otherwise
   */
  bool processItem(const T& e) noexcept {
    ++(_histogram[e]);
    ++_total_processed_elements;
    if ((_total_processed_elements % _window_size) == 0) {
      _decreaseAllFrequencies();
      return true;
    }
    return false;
  }

  /**
   * Process N elements from begin iterator without copying it to a
   * temporary buffer. The caller is responsible for providing enough
   * elements
   */
  template <typename Iter>
  void processWindow(Iter it) noexcept {
    for (unsigned int cnt = 0; cnt < _window_size; ++cnt) {
      ++(_histogram[*it]);
      ++it;
    }
    _total_processed_elements += _window_size;
    _decreaseAllFrequencies();
  }

  /**
   * returns the final histogram containing only elements
   * with count exceeding fN - eN. The type of the container
   * the final histogram is stored in can be set using the
   * template parameter.
   */
  template <typename ContainerT = std::map<T, size_t>>
  ContainerT computeOutput() const noexcept {
    ContainerT result;

    const int threshold =
        static_cast<int>((_frequency * _total_processed_elements) -
                         (_error * _total_processed_elements));

    std::copy_if(
        _histogram.begin(), _histogram.end(), std::back_inserter(result),
        [&](const std::pair<T, size_t>& el) { return el.second >= threshold; });
    return result;
  }

  template <typename ContainerT = std::vector<T>>
  ContainerT computeOutputKeys() const noexcept {
    ContainerT result;

    const int threshold = (_frequency * _total_processed_elements) -
                          (_error * _total_processed_elements);

    for (const auto& el : _histogram) {
      if (el.second >= threshold) {
        result.push_back(el.first);
      }
    }
    return result;
  }

  State getState() const noexcept {
    return State{_frequency, _error, _window_size, _total_processed_elements};
  }

  void reset() {
    _histogram.clear();
    _total_processed_elements = 0;
  }

 private:
  void _decreaseAllFrequencies() noexcept {
    auto it = _histogram.begin();
    while (it != _histogram.end()) {
      if (it->second == 1) {
        it = _histogram.erase(it);
      } else {
        --(it->second);
        ++it;
      }
    }
  }
};
}  // namespace drace
#endif
