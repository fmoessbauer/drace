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

#include <detector/detector_if.h>
#include <string>
#include <iostream>

static unsigned        num_races = 0;
static detector::Race  last_race;

class DetectorTest : public ::testing::Test {

public:
	// each callback-call increases num_races by two
	static void callback(const detector::Race* race) {
        // copy race to let testing code inspect it later
        last_race = *race;
		++num_races;
	}

	DetectorTest() {
		num_races = 0;
        last_race = {};
		if (std::string(detector::name()) != "TSAN") {
			const char * _argv = "drace-tests.exe";
			detector::init(1, &_argv, callback);
		}
	}

	~DetectorTest() {
		if (detector::name() != "TSAN") {
			detector::finalize();
		}
	}

protected:
	// TSAN can only be initialized once, even after a finalize
	static void SetUpTestCase() {
		std::cout << "Detector: " << detector::name() << std::endl;
		if (std::string(detector::name()) == "TSAN") {
			const char * _argv = "drace-tests-tsan.exe";
			detector::init(1, &_argv, callback);
		}
	}

	static void TearDownTestCase() {
		if (detector::name() == "TSAN") {
			detector::finalize();
		}
	}
};
