 # DRace, a dynamic data race detector
 #
 # Copyright 2018 Siemens AG
 #
 # Authors:
 #   Philip Harr <philip.harr@siemens.com>
 #
 # SPDX-License-Identifier: MIT

add_library(race_filter_test OBJECT "race-filter-test.cpp" "../src/race-filter.cpp")
target_link_libraries(race_filter_test gtest "drace-common")
target_include_directories(race_filter_test PUBLIC "../include")

set_property(GLOBAL APPEND PROPERTY DRACE_TEST_SUITES race_filter_test)
