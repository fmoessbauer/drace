#pragma once

#undef min
#undef max

#include <array>

#include "ringbuffer.hpp"

namespace ipc {
	namespace event {
		enum class Type : uint8_t {
			NONE,
			MEMREAD,
			MEMWRITE,
			MUTEX,
			ALLOCATION,
			FORKJOIN,
			DETACHFINISH
		};

		struct MemAccess {
			uint32_t thread_id;
			uint32_t stacksize;
			uint64_t addr;
			std::array<uint64_t, 4> callstack;
		};

		struct Mutex {
			unsigned long thread_id;
			uint64_t addr;
			int recursive;
			bool write;
			bool acquire;
		};

		struct Allocation {
			unsigned long thread_id;
			uint64_t addr;
			size_t size;
			bool alloc;
		};

		struct ForkJoin {
			unsigned long parent;
			unsigned long child;
			bool fork;
		};

		struct DetachFinish {
			unsigned long thread_id;
		};

		struct BufferEntry {
			Type type{ Type::NONE };
			char buffer[sizeof(MemAccess)];
		};

		template<typename T, class... Args>
		struct GenericEntry {
			Type type{ Type::NONE };
			T buffer;
		
			GenericEntry(Type t, Args&&... args)
				: type(t), buffer(std::forward<Args>(args)...) { }
		};
	}

	using queue_t = ipc::Ringbuffer<ipc::event::BufferEntry, 1 << 20, false, 64>;
}