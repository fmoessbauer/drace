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

#include <memory>
#include "ipc/SharedMemory.h"
#include "ipc/SMData.h"

/// Managed Stack Resolver
namespace msr {

	/** Controls the runstate of DRace by externally setting DRace' control block */
	class Controller {
	public:
		/// Pointer type of shared memory control block
		using PSHMCB = std::shared_ptr<ipc::SharedMemory<ipc::ClientCB, false>>;
	private:
		/// Pointer to DRace SHM controlblock
		PSHMCB _pshmcb;
		bool   _active{ false };

	public:
		Controller() = delete;
		explicit Controller(const PSHMCB & pshmcb)
			: _pshmcb(pshmcb) { }

		~Controller();

		Controller(const Controller &) = delete;
		Controller(Controller &&) = default;

		Controller & operator=(const Controller &) = delete;
		Controller & operator=(Controller &&) = default;

		/** Start the interaction loop */
		void start();

		/** Stop the interaction loop */
		void stop();

		/** set detector state to enabled */
		void enable_detector();
		/** set detector state to disabled */
		void disable_detector();
		/** set sampling rate of DRace */
		void set_samplingrate(int s);

	private:
		void _error_no_cb();
	};
}