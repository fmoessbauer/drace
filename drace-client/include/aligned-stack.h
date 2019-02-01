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

#include "aligned-buffer.h"

namespace drace {
	/** Trivial Stack implementation consisting of a buffer and an entries counter */
	template<typename T, size_t alignment>
	class AlignedStack : public AlignedBuffer<T, alignment> {
	public:
		unsigned long entries{ 0 };

		inline void push(const T & val) {
			data[entries++] = val;
		}

		inline T & pop() {
			return array[--entries];
		}

		inline T* begin() const {
			return array;
		}
		inline const T* cbegin() const {
			return array;
		}

		inline T* end() const {
			return array + entries;
		}
		inline const T* cend() const {
			return array + entries;
		}
	};
}
