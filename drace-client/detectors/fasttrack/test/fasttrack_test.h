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

#include "../include/fasttrack.h"
#include <shared_mutex>

class FasttrackTest: public ::testing::Test {

  FasttrackTest() {
    drace::detector::Fasttrack<std::shared_mutex> ft;
    const char *p = "";
    ft.init(0, &p, clb);
  }

  ~FasttrackTest()  {
    
  }

  void SetUp()  {

  }

  void TearDown()  {

  }

  static void clb(const Detector::Race* ptr){}
 
};
