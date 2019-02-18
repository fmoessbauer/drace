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

#include <cstdint>
#include <chrono>

#include "ipc/ExtsanData.h"
#include "LoggerTypes.h"
#include "sparsepp/spp.h"

namespace msr {
	class QueueHandler {
		using Queue_t = ipc::queue_t;
		using Map_t = spp::sparse_hash_map<uint64_t, void*>;

		ipc::QueueMetadata & _qmeta;

		// for stats
		using tp_t = decltype(std::chrono::system_clock::now());
		tp_t _last_sample;
		std::array<uint64_t, ipc::QueueMetadata::num_queues> _events{ 0 };
		std::array<uint64_t, ipc::QueueMetadata::num_queues> _last_evtcnt{ 0 };

		std::array<Map_t, ipc::QueueMetadata::num_queues> _tids;

	public:
		explicit QueueHandler(ipc::QueueMetadata & qmeta)
			: _qmeta(qmeta)
		{
			logger->debug("ringbuffer size {}MB", 
				(Queue_t::slots * sizeof(Queue_t::value_type)) / (1024*1024));
			init_detector();
		}

		/* TODO */
		void start();

		void process(int queueid);

		template<typename Duration = std::chrono::seconds>
		void monitor(Duration dur = std::chrono::seconds(2))
		{
			while (true) {
				print_stats();
				std::this_thread::sleep_for(dur);
			}
		}

	private:
		void init_detector();
		void print_stats();

		/**
		 * split address at 32-bit boundary (zero above)
		 * TODO: TSAN seems to only support 32 bit addresses!!!
		 */
		template <typename T>
		constexpr uint64_t lower_half(T addr) {
			return (addr & 0x00000000FFFFFFFF);
		}

		void _acquire(ipc::event::Mutex*, int queueid);
		void _release(ipc::event::Mutex*, int queueid);
		void _happens_before();
		void _happens_after();
		void _read(ipc::event::MemAccess*, int queueid);
		void _write(ipc::event::MemAccess*, int queueid);
		void _allocate(ipc::event::Allocation*, int queueid);
		void _free(ipc::event::Allocation*, int queueid);
		void _fork(ipc::event::ForkJoin*, int queueid);
		void _join(ipc::event::ForkJoin*, int queueid);
		void _detach(ipc::event::DetachFinish*, int queueid);
		void _finish(ipc::event::DetachFinish*, int queueid);
	};
}
