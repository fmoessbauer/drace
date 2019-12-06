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

using ::testing::UnitTest;
using drace::RaceFilter;

/**
 * Check the supression of an exact-match race
 */
TEST(FilterTest, StaticSupression) {

    std::string file = "race_suppressions.txt";

    // create fake detector race
    Detector::Race r;
    r.first.stack_size = 1;
    r.first.stack_trace[0] = 0x42;

    // create dummy symbol
    drace::symbol::SymbolLocation symbol_location;
    symbol_location.file = "FilterTest.cpp";
    symbol_location.mod_name = "FilterTestModule";
    symbol_location.sym_name = "main";

    // this should be supressed later
    auto symbol_loc_lp = symbol_location;
    symbol_loc_lp.sym_name = "std::_LaunchPad<>::_Go";

    drace::race::ResolvedAccess ra(r.first);
    drace::race::ResolvedAccess rb(r.second);
    drace::race::ResolvedAccess r_launchpad(r.first);
    ra.resolved_stack.push_back(symbol_location);
    rb.resolved_stack.push_back(symbol_location);
    r_launchpad.resolved_stack.push_back(symbol_loc_lp);

    drace::race::DecoratedRace race_real(ra, rb, std::chrono::milliseconds(1000));
    // create a false positive race
    drace::race::DecoratedRace race_launchpad(r_launchpad, rb, std::chrono::milliseconds(1000));

    RaceFilter filter(file);

    ASSERT_FALSE(filter.check_suppress(race_real));
    ASSERT_TRUE(filter.check_suppress(race_launchpad));
}
