#include "QueueHandler.h"
#include "LoggerTypes.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <thread>

#include <windows.h>
#include "tsan-if.h"

namespace msr {
	void QueueHandler::start() {
		logger->info("queue handler started");

		for (int i = 0; i < ipc::QueueMetadata::num_queues; ++i) {
			std::thread t(&QueueHandler::process, this, i);
			t.detach();
		}
	}

	void QueueHandler::process(int queueid) {
		using ipc::event::Type;
		auto pitem = std::make_unique<ipc::event::BufferEntry>();

		SetThreadAffinityMask(GetCurrentThread(), 1 << (queueid % std::thread::hardware_concurrency()));

		logger->info("process messages on queue {}", queueid);
		while (true) {
			auto & queue = _qmeta.queues[queueid];
			if (queue.remove(pitem.get())) {
				++_events[queueid];
				switch (pitem->type) {
				case Type::ACQUIRE:
					_acquire((ipc::event::Mutex*) pitem->buffer, queueid);
					break;
				case Type::RELEASE:
					_release((ipc::event::Mutex*) pitem->buffer, queueid);
					break;
				case Type::MEMREAD:
					_read((ipc::event::MemAccess*) pitem->buffer, queueid);
					break;
				case Type::MEMWRITE:
					_write((ipc::event::MemAccess*) pitem->buffer, queueid);
					break;
				case Type::ALLOCATION:
					_allocate((ipc::event::Allocation*) pitem->buffer, queueid);
					break;
				case Type::FREE:
					_free((ipc::event::Allocation*) pitem->buffer, queueid);
					break;
				case Type::FORK:
					_fork((ipc::event::ForkJoin*) pitem->buffer, queueid);
					break;
				case Type::JOIN:
					break;
				case Type::DETACH:
					break;
				case Type::FINISH:
					break;
				}
				//std::cout << "Got " << (short)pitem->type << std::endl;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
	}

	static void callback(__tsan_race_info* raceInfo, void* params) {
		logger->info("RACE");
	}

	void QueueHandler::init_detector() {
		__tsan_init_simple(callback, nullptr);
		logger->debug("initialized tsan");
	}

	void QueueHandler::print_stats() {
		// This is racy, but only rough estimates are needed
		auto timediff = std::chrono::system_clock::now() - _last_sample;
		_last_sample = std::chrono::system_clock::now();

		for (int i = 0; i < ipc::QueueMetadata::num_queues; ++i) {
			auto evtdiff = _events[i] - _last_evtcnt[i];
			_last_evtcnt[i] = _events[i];
			auto & queue = _qmeta.queues[i];
			double level = static_cast<double>(queue.readAvailable()) / Queue_t::slots;

			// we are interesed in MB/s = B/us throughput 
			auto proc_byte = evtdiff * sizeof(Queue_t::value_type);
			double per_us_byte = static_cast<double>(proc_byte) / std::chrono::duration_cast<std::chrono::microseconds>(timediff).count();
			double per_s_elem = (static_cast<double>(evtdiff) / std::chrono::duration_cast<std::chrono::microseconds>(timediff).count());
			logger->debug("queue {} throughput {:.2f}MB/s, {:.2f}MElem/s, level(read) {:03.2f}%",
				i, per_us_byte, per_s_elem, level * 100);
		}
	}

	// ----- TSAN Messages -----
	void QueueHandler::_acquire(ipc::event::Mutex* entry, int queueid) {
		void* thr = _tids[queueid][entry->thread_id];
		logger->trace("thr {}@{}", entry->thread_id, thr);
		if (entry->write) {
			__tsan_MutexLock(thr, 0, (void*)lower_half(entry->addr), entry->recursive, false);
		}
		else {
			__tsan_MutexReadLock(thr, 0, (void*)lower_half(entry->addr), false);
		}
	}
	void QueueHandler::_release(ipc::event::Mutex* entry, int queueid) {
		void* thr = _tids[queueid][entry->thread_id];
		if (entry->write) {
			__tsan_MutexUnlock(thr, 0, (void*)lower_half(entry->addr), false);
		}
		else {
			__tsan_MutexReadUnlock(thr, 0, (void*)lower_half(entry->addr));
		}
	}
	void QueueHandler::_read(ipc::event::MemAccess* entry, int queueid) {
		logger->trace("read: tid {}, addr {}, stacksize {}", entry->thread_id, (void*)entry->addr, entry->stacksize);

		void* thr = _tids[queueid][entry->thread_id];
		if (nullptr == thr) return;
		__tsan_read(thr, (void*)lower_half(entry->addr), kSizeLog1, entry->callstack.data(), entry->stacksize);
	}
	void QueueHandler::_write(ipc::event::MemAccess* entry, int queueid) {
		logger->trace("write: tid {}, addr {}, stacksize {}", entry->thread_id, (void*)entry->addr, entry->stacksize);

		void* thr = _tids[queueid][entry->thread_id];
		if (nullptr == thr) return;
		__tsan_write(thr, (void*)lower_half(entry->addr), kSizeLog1, entry->callstack.data(), entry->stacksize);
	}
	void QueueHandler::_allocate(ipc::event::Allocation* entry, int queueid) {
		logger->trace("allocate: tid {}, addr {}, size {}", entry->thread_id, entry->addr, entry->size);
		__tsan_malloc_use_user_tid(entry->thread_id, 0x42, lower_half(entry->addr), entry->size);
	}

	void QueueHandler::_free(ipc::event::Allocation* entry, int queueid) {
		logger->trace("free: addr {}, size {}", entry->addr, entry->size);
		__tsan_reset_range(lower_half(entry->addr), entry->size);
	}

	void QueueHandler::_fork(ipc::event::ForkJoin* entry, int queueid) {
		void* thr = __tsan_create_thread(entry->child);
		logger->trace("Fork child {}@{}", entry->child, thr);
		_tids[queueid][entry->child] = thr;
	}
}