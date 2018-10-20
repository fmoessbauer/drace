#include "QueueHandler.h"
#include "LoggerTypes.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <thread>

#include <windows.h>

namespace msr {
	void QueueHandler::start() {
		logger->info("queue handler started");

		auto pitem = std::make_unique<ipc::event::BufferEntry>();
		unsigned long misses;

		//SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(1) << 1);

		while (true) {
			if (_queue.remove(pitem.get())) {
				++_events;
				//std::cout << "Got " << (short)pitem->type << std::endl;
			}
			else {
				if (++misses > 100) {
					misses = 0;
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			}
		}
	}

	void QueueHandler::print_stats() {
		// This is racy, but only rough estimates are needed
		auto evtdiff = _events - _last_evtcnt;
		_last_evtcnt = _events;
		auto timediff = std::chrono::system_clock::now() - _last_sample;
		_last_sample = std::chrono::system_clock::now();
		double level = static_cast<double>(_queue.readAvailable()) / Queue_t::slots;

		// we are interesed in MB/s = B/us throughput 
		auto proc_byte = evtdiff * sizeof(Queue_t::value_type);
		double per_us_byte = static_cast<double>(proc_byte) / std::chrono::duration_cast<std::chrono::microseconds>(timediff).count();
		double per_s_elem = (static_cast<double>(evtdiff) / std::chrono::duration_cast<std::chrono::microseconds>(timediff).count());
		logger->debug("queue throughput {:.2f}MB/s, {:.2f}MElem/s, level(read) {:03.2f}%",
			per_us_byte, per_s_elem, level*100);
	}
}