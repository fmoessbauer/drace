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

#include "aligned-buffer.h"

namespace drace {
	/// Trivial Stack implementation consisting of a buffer and an entries counter
	template<typename T, size_t alignment>
	class AlignedStack : public AlignedBuffer<T, alignment> {
	public:
		unsigned long entries{ 0 };

		inline void push(const T & val) {
			this.data[entries++] = val;
		}

		inline T & pop() {
			return this.data[--entries];
		}

		inline T* begin() const {
			return this.data;
		}
		inline const T* cbegin() const {
			return this.data;
		}

		inline T* end() const {
			return this.data + entries;
		}
		inline const T* cend() const {
			return this.data + entries;
		}
	};
}
