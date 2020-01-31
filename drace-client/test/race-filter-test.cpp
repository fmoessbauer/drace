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
#include "race-filter.h"
#include <fstream>

using ::testing::UnitTest;
using drace::RaceFilter;

/**
 * Check the supression of an exact-match race
 */
//top of stack race suppression
TEST(FilterTest, StaticSupression) {

    std::string file = "bin/race_suppressions.txt";

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

//checks our 'regex' characters
TEST(FilterTest, RegEx){
    // create fake detector race
    Detector::Race r;
    r.first.stack_size = 1;
    r.first.stack_trace[0] = 0x42;

    // create dummy symbol
    drace::symbol::SymbolLocation symbol_location;
    symbol_location.file = "FilterTest.cpp";
    symbol_location.mod_name = "FilterTestModule";
    //no suppression
    symbol_location.sym_name = "maintenance";

    //suppression
    auto symbol_loc_1 = symbol_location;
    symbol_loc_1.sym_name = "std::example::function";

    //suppression
    auto symbol_loc_2 = symbol_location;
    symbol_loc_2.sym_name = "main";

    // suppression
    auto symbol_loc_3 = symbol_location;
    symbol_loc_3.sym_name = "sup_add";

    drace::race::ResolvedAccess r0(r.first);
    drace::race::ResolvedAccess r1(r.first);
    drace::race::ResolvedAccess r2(r.first);
    drace::race::ResolvedAccess r3(r.first);
    //drace::race::ResolvedAccess r_empty(r.second);

    r0.resolved_stack.push_back(symbol_location);
    r1.resolved_stack.push_back(symbol_loc_1);
    r2.resolved_stack.push_back(symbol_loc_2);
    r3.resolved_stack.push_back(symbol_loc_3);

    drace::race::DecoratedRace race0(r0, r0, std::chrono::milliseconds(1000));
    drace::race::DecoratedRace race1(r1, r0, std::chrono::milliseconds(1000));
    drace::race::DecoratedRace race2(r2, r0, std::chrono::milliseconds(1000));
    drace::race::DecoratedRace race3(r3, r0, std::chrono::milliseconds(1000));

    std::stringbuf buf("race_tos:add\nrace_tos:^main$\nrace_tos:std::*::function\n");
    std::istream test_file(&buf);
    RaceFilter filter(test_file);


    ASSERT_FALSE(filter.check_suppress(race0));
    ASSERT_TRUE(filter.check_suppress(race1));
    ASSERT_TRUE(filter.check_suppress(race2));
    ASSERT_TRUE(filter.check_suppress(race3));
}

//in stack race suprression
TEST(FilterTest, In_Stack_Race){
    // create fake detector race
    Detector::Race r;
    r.first.stack_size = 2;
    r.first.stack_trace[0] = 0x42;
    r.first.stack_trace[1] = 0x43;

    // create dummy symbol
    drace::symbol::SymbolLocation symbol_location;
    symbol_location.file = "FilterTest.cpp";
    symbol_location.mod_name = "FilterTestModule";
    //no suppression
    symbol_location.sym_name = "example";

    //suppression
    auto symbol_loc_1 = symbol_location;
    symbol_loc_1.sym_name = "add";

    drace::race::ResolvedAccess r0(r.first);
    drace::race::ResolvedAccess r1(r.first);

    r0.resolved_stack.push_back(symbol_loc_1);
    r0.resolved_stack.push_back(symbol_location);

    r1.resolved_stack.push_back(symbol_location);
    r1.resolved_stack.push_back(symbol_location);

    drace::race::DecoratedRace race0(r0, r0, std::chrono::milliseconds(1000));
    drace::race::DecoratedRace race1(r1, r1, std::chrono::milliseconds(1000));

    std::stringbuf buf("race:^add$");
    std::istream test_file(&buf);
    RaceFilter filter(test_file);

    ASSERT_TRUE(filter.check_suppress(race0));
    ASSERT_FALSE(filter.check_suppress(race1));
 }
