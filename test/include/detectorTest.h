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

#include <detector/Detector.h>
#include <util/WindowsLibLoader.h>
#include <string>
#include <iostream>
#include <Windows.h>

static unsigned        num_races = 0;
static Detector::Race  last_race;

class DetectorTest : public ::testing::TestWithParam<const char*>{

protected:
    static util::WindowsLibLoader _libtsan;
    static util::WindowsLibLoader _libdummy;

    static Detector * _dettsan;
    static Detector * _detdummy;

public:
    Detector * detector;

	// each callback-call increases num_races by two
	static void callback(const Detector::Race* race) {
        // copy race to let testing code inspect it later
        last_race = *race;
		++num_races;
	}

	DetectorTest() {
		num_races = 0;
        last_race = {};

        if (std::string(GetParam()).compare("tsan") == 0) {
            detector = _dettsan;
        }
        else if (std::string(GetParam()).compare("dummy") == 0) {
            detector = _detdummy;
        }
	}
    ~DetectorTest() {
        detector = nullptr;
    }

	// TSAN can only be initialized once, even after a finalize
	static void SetUpTestCase() {
        _libtsan.load("drace.detector.tsan.dll");
        _libdummy.load("drace.detector.dummy.dll");

        decltype(CreateDetector)* create_tsan = _libtsan["CreateDetector"];
        decltype(CreateDetector)* create_dummy = _libdummy["CreateDetector"];

        _dettsan = create_tsan();
        _detdummy = create_dummy();

    	const char * _argv = "drace-tests.exe";
		_dettsan->init(1, &_argv, callback);
        _detdummy->init(1, &_argv, callback);
	}

	static void TearDownTestCase() {
        _dettsan->finalize();
        _detdummy->finalize();
	}
};
