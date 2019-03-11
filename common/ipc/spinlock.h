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

#include <atomic>
#include <thread>
#ifdef DEBUG
#include <iostream>
#endif

namespace ipc {
	/**
	* Simple mutex implemented as a spinlock
	* implements interface of std::mutex
	*/
	class spinlock {
        std::atomic_flag _flag{false};
	public:
		inline void lock() noexcept
		{
			unsigned cnt = 0;
			while (_flag.test_and_set(std::memory_order_acquire)) {
				if (++cnt == 100) {
					// congestion on the lock
					std::this_thread::yield();
                    cnt = 0;
#ifdef DEBUG    
					std::cout << "spinlock congestion" << std::endl;
#endif          
				}
			}
		}

		inline bool try_lock() noexcept
		{
			return !(_flag.test_and_set(std::memory_order_acquire));
		}

		inline void unlock() noexcept
		{
			_flag.clear(std::memory_order_release);
		}
	};

} // namespace ipc
