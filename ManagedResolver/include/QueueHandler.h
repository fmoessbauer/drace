#pragma once

#include <cstdint>
#include <chrono>

#include "ipc/ExtsanData.h"
#include "LoggerTypes.h"

namespace msr {
	class QueueHandler {
		using Queue_t = ipc::queue_t;

		Queue_t & _queue;
		uint64_t  _events{ 0 };

		// for stats
		using tp_t = decltype(std::chrono::system_clock::now());
		tp_t _last_sample;
		uint64_t _last_evtcnt{ 0 };

	public:
		explicit QueueHandler(Queue_t & queue)
			: _queue(queue)
		{
			logger->debug("ringbuffer size {}MB", 
				(Queue_t::slots * sizeof(Queue_t::value_type)) / (1024*1024));
		}

		/* TODO */
		void start();

		template<typename Duration = std::chrono::seconds>
		void monitor(Duration dur = std::chrono::seconds(2))
		{
			while (true) {
				print_stats();
				std::this_thread::sleep_for(dur);
			}
		}

	private:
		void print_stats();
	};
}
