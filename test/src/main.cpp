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

#include "gtest/gtest.h"
#include "globals.h"

using ::testing::UnitTest;

int    t_argc;
char** t_argv;

int main(int argc, char ** argv) {
	::testing::InitGoogleTest(&argc, argv);

	t_argc = argc;
	t_argv = argv;

	int ret = RUN_ALL_TESTS();
	return ret;
}