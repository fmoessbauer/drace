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

#include "ipc/SyncSHMDriver.h"
#include <iostream>

#include <dr_api.h>

namespace ipc {
	/** Provides methods for synchronized access to a SyncSHMDriver 
	* An object of this type can be passed into a \c std::lock_guard
	*/
	template<bool SENDER, bool NOTHROW = true>
	class MtSyncSHMDriver : public SyncSHMDriver<SENDER, NOTHROW> {
		void * _mx;

	public:
		MtSyncSHMDriver(const char* shmkey, bool create)
			: SyncSHMDriver(shmkey, create)
		{
			_mx = dr_mutex_create();
		}

		~MtSyncSHMDriver() {
			dr_mutex_destroy(_mx);
		}

		inline void lock() {
			dr_mutex_lock(_mx);
		}

		inline void unlock() {
			dr_mutex_unlock(_mx);
		}
	};
}