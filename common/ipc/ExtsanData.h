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
			ACQUIRE,
			RELEASE,
			ALLOCATION,
			FREE,
			FORK,
			JOIN,
			DETACH,
			FINISH
		};

		struct MemAccess {
			uint32_t thread_id;
			uint32_t stacksize;
			uint64_t addr;
			std::array<uint64_t, 16> callstack;
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
			uint64_t pc;
			uint64_t addr;
			size_t size;
		};

		struct ForkJoin {
			unsigned long parent;
			unsigned long child;
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

	struct QueueMetadata {
		static constexpr unsigned num_queues{ 2 };
		unsigned next_queue{ 0 };
		queue_t queues[num_queues];
	};
}