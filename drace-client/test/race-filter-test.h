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

#include "../include/race-filter.h"
#include <string>

class RaceFilterTest: public ::testing::Test {

  RaceFilterTest() {
    std::string filename = "testfile.txt";
    drace::RaceFilter filter(filename);
  }

  ~RaceFilterTest()  {
    
  }

  void SetUp()  {

  }

  void TearDown()  {

  }

 
};
