#pragma once

#include "aligned-buffer.h"

/* Trivial Stack implementation consisting of a buffer and an entries counter */
template<typename T, size_t alignment>
class AlignedStack : public AlignedBuffer<T, alignment> {
public:
	unsigned long entries{ 0 };

	inline void push(const T & val) {
		array[entries++] = val;
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
