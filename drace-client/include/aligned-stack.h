#pragma once

#include "aligned-buffer.h"

/* Trivial Stack implementation consisting of a buffer and an entries counter */
template<typename T, size_t alignment>
class AlignedStack : public AlignedBuffer<T, alignment> {
public:
	unsigned long entries{ 0 };
};
