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
#include "race-filter-test.h"
#include "string"

using ::testing::UnitTest;
TEST(FilterTest, parser) {

    std::string file = "C:/Users/z00435hk/dev/drace/drace-client/test/suppressions.txt";
    drace::RaceFilter filter(file);
    filter.print_list();

}