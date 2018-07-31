#pragma once

#include <dr_api.h>

/* Aligned Buffer, allocated in DR's thread local storage of instantiator */
template<typename T, size_t alignment>
class AlignedBuffer {
	/* unaligned buffer begin */
	void *   _mem{ nullptr };

	/* size of the buffer in bytes */
	size_t   _size_in_bytes{ 0 };

	/* dr context at allocation time. Use this context for deallocation */
	void *   _alloc_ctx;

public:
	using self_t = AlignedBuffer<T, alignment>;
	T * array{ nullptr };

public:
	AlignedBuffer() = default;
	AlignedBuffer(const self_t & other) = delete;
	AlignedBuffer(self_t && other) = default;

	self_t & operator= (const self_t & other) = delete;
	self_t & operator= (self_t && other) = default;

	explicit AlignedBuffer(const size_t & capacity, void* drcontext = dr_get_current_drcontext())
		: _alloc_ctx(drcontext)
	{
		allocate(_alloc_ctx, capacity);
	}

	/* Clears the old buffer and allocates a new one with the specified capacity */
	void resize(size_t capacity, void* drcontext = dr_get_current_drcontext()) {
		_alloc_ctx = drcontext;
		deallocate(_alloc_ctx);
		allocate(_alloc_ctx, capacity);
	}

	/* Frees the allocated tls, hence has to be called in same thread as allocation */
	~AlignedBuffer() {
		deallocate(_alloc_ctx);
	}

	/* deallocate this buffer using the provided drcontext.
	 * If no context is provided, the context at allocation time is used
	 */
	void deallocate(void* drcontext = nullptr) {
		if (_size_in_bytes != 0) {
			if (nullptr == drcontext) {
				drcontext = _alloc_ctx;
			}
			dr_thread_free(drcontext, _mem, _size_in_bytes);
			_size_in_bytes = 0;
			array = nullptr;
		}
	}

private:
	void allocate(void* drcontext, size_t capacity) {
		if (capacity != 0) {
			_size_in_bytes = capacity * sizeof(T) + alignment;
			// copy this as value is modified by std::align
			size_t space_size = _size_in_bytes;
			_mem = dr_thread_alloc(drcontext, _size_in_bytes);
			array = (T*)std::align(alignment, capacity, _mem, space_size);
			DR_ASSERT(((uint64_t)array % alignment) == 0);
		}
	}
};
