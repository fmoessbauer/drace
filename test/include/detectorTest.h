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
#include <Windows.h>

static unsigned        num_races = 0;
static Detector::Race  last_race;

class DetectorTest : public ::testing::Test {

protected:
    static HMODULE detector_lib;

public:
    static Detector * detector;

	// each callback-call increases num_races by two
	static void callback(const Detector::Race* race) {
        // copy race to let testing code inspect it later
        last_race = *race;
		++num_races;
	}

	DetectorTest() {
		num_races = 0;
        last_race = {};

		if (std::string(detector->name()) != "TSAN") {
			const char * _argv = "drace-tests.exe";
			detector->init(1, &_argv, callback);
		}
	}

	~DetectorTest() {
		if (std::string(detector->name()) != "TSAN") {
			detector->finalize();
		}
	}

protected:
	// TSAN can only be initialized once, even after a finalize
	static void SetUpTestCase() {
        detector_lib = LoadLibrary("drace.detector.tsan.dll");
        auto create_detector = reinterpret_cast<decltype(CreateDetector)*>(GetProcAddress(detector_lib, "CreateDetector"));
        detector = create_detector();

		std::cout << "Detector: " << detector->name() << std::endl;
		if (std::string(detector->name()) == "TSAN") {
			const char * _argv = "drace-tests-tsan.exe";
			detector->init(1, &_argv, callback);
		}
	}

	static void TearDownTestCase() {
		if (std::string(detector->name()) == "TSAN") {
			detector->finalize();
		}
        detector = nullptr;
        FreeLibrary(detector_lib);
	}
};
