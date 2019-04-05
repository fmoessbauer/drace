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

#include "Controller.h"
#include "LoggerTypes.h"

#include <thread>
#include <iostream>
#include <string>

namespace msr {
	Controller::~Controller() {
		_active = false;
	}

	void Controller::start() {
        std::cout << "Change detector state using the following keys:" << std::endl
                  << "e\tenable detector" << std::endl
                  << "d\tdisable detector" << std::endl
                  << "s\tset sampling rate" << std::endl;

		logger->info("started interactive controller");
		std::string valstr;

		_active = true;
		while (_active) {
			// dead simple parser (unfortunately blocks for IO, no easy workaround)
			switch (char c = std::cin.get()) {
			case 'e':
				enable_detector();
				break;
			case 'd':
				disable_detector();
				break;
			case 's':
				std::getline(std::cin, valstr);
				try {
					int val = std::stoi(valstr);
					set_samplingrate(val);
				}
				catch (std::invalid_argument) {
					logger->warn("invalid argument");
					break;
				}
				break;
			case '\n':
				break;
			default:
				logger->warn("unknown input {}", c);
			}
		}
	}

	void Controller::stop() {
		if (_active) {
			logger->warn("Press any key to quit");
		}
		_active = false;
	}

	void Controller::enable_detector() {
		auto cb = _pshmcb->get();
		if (cb) {
			if (cb->enabled) return;
			cb->enabled.store(true, std::memory_order_relaxed);
			logger->info("enabled detector");
			return;
		}
		_error_no_cb();
	}

	void Controller::disable_detector() {
		auto cb = _pshmcb->get();
		if (cb) {
			if (!cb->enabled) return;
			cb->enabled.store(false, std::memory_order_relaxed);
			logger->info("disabled detector");
			return;
		}
		_error_no_cb();
	}

	void Controller::set_samplingrate(int s) {
		auto cb = _pshmcb->get();
		if (cb) {
			cb->sampling_rate.store(s, std::memory_order_relaxed);
			logger->info("set sampling rate to {}", s);
			return;
		}
		_error_no_cb();
	}

	void Controller::_error_no_cb() {
		logger->error("control block is not available");
	}
}
