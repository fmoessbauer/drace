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