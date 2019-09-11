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

#include <gtest/gtest.h>

#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "globals.h"

class Integration : public ::testing::Test {
private:
	static std::string drrun;
	static std::string drclient;
	static bool verbose;

	std::string logfile;

public:
	Integration() {
		logfile = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		std::replace(logfile.begin(), logfile.end(), '/', '_');
		logfile += ".log";
	}

	~Integration() {
		std::remove(logfile.c_str());
	}

	void run(const std::string & client_args, const std::string & exe, int min, int max) {
		std::stringstream command;
		command << drrun << " -c " << drclient << " "
				<< client_args << " -- " << "test/" << exe
			    << " > " << logfile << " 2>&1";
		if(verbose)
			std::cout << ">> Issue Command: " << command.str() << std::endl;

		std::system(command.str().c_str());

		// TODO: parse output
		std::ifstream filestream(logfile);
		std::stringstream file_content;
		file_content << filestream.rdbuf();
		std::string output(file_content.str());

		std::regex expr("found (\\d+) possible data-races");
		std::smatch races_match;
		if (std::regex_search(output, races_match, expr)) {
			std::string num_races_str;
			std::copy(races_match[1].first, races_match[1].second, std::back_inserter(num_races_str));
			
			if(verbose)
				std::cout << ">>> Detected Races: " << num_races_str << std::endl;

			int num_races = std::stoi(num_races_str);
			if (num_races < min || num_races > max) {
				ADD_FAILURE() << "Expected [" << min << "," << max << "] Races, found " << num_races
					<< " in " << exe;
                if (verbose) {
                    std::cout << output << std::endl;
                }
			}
		}
		else {
			ADD_FAILURE() << "Race-Summary not found";
		}
	}

	// TSAN can only be initialized once, even after a finalize
	static void SetUpTestCase() {
		for (int i = 1; i < t_argc; ++i) {
			if (strncmp(t_argv[i], "--dr", 8) == 0 && i<(t_argc-1)) {
				drrun = t_argv[i + 1];
			}
			else if (strncmp(t_argv[i], "-v", 4) == 0) {
				verbose = true;
			}
		}

		if (verbose) {
			std::cout << "> Use DynamoRio in: " << drrun << std::endl;
			std::cout << "> Use DRace in:     " << drclient << std::endl;
		}
		// TODO: Check if drrun is present
		std::string dr_test(drrun + " -h > dr_avail.log");
		int ret = std::system(dr_test.c_str());
		if (ret) {
			throw std::exception("drrun.exe not found");
		}
		std::ifstream f(drclient.c_str());
		if (!f.good()) {
			throw std::exception("DRace not found");
		}
		f.close();
	}

	static void TearDownTestCase() {
		std::remove("dr_avail.log");
	}
};

class DR : public Integration, public ::testing::WithParamInterface<const char*> {

};
