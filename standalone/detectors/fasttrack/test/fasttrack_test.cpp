/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Philip Harr <philip.harr@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "gtest/gtest.h"
#include "fasttrack_test.h"


using ::testing::UnitTest;

TEST(FasttrackTest, Test1) {
	ASSERT_EQ(1,1);
}