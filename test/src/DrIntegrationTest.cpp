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

#include "DrIntegrationTest.h"

#include <iostream>
#include <string>

#ifdef XML_EXPORTER
#include <tinyxml2.h>
#include <cstdio>
#endif

std::string DrIntegration::drrun = "drrun.exe";
std::string DrIntegration::drclient = "drace-client/drace-client.dll";
bool DrIntegration::verbose = false;

// Test cases

TEST_P(FlagMode, ConcurrentInc) {
	run(GetParam(), "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
}
TEST_P(FlagMode, IncMutex) {
	run(GetParam(), "mini-apps/inc-mutex/gp-inc-mutex.exe", 0, 0);
}
TEST_P(FlagMode, LockKinds) {
	run(GetParam(), "mini-apps/lock-kinds/gp-lock-kinds.exe", 0, 0);
}
TEST_P(FlagMode, EmptyMain) {
	run(GetParam(), "mini-apps/empty-main/gp-empty-main.exe", 0, 0);
}
TEST_P(FlagMode, Atomics) {
	run(GetParam(), "mini-apps/atomics/gp-atomics.exe", 0, 0);
}
// TODO: Redesign test:
//       Currently, many runs miss the race as the race often
//       does not occur.
//TEST_P(FlagMode, RacyAtomics) {
//	run(GetParam(), "mini-apps/atomics/gp-atomics.exe racy", 1, 10);
//}
TEST_P(FlagMode, Annotations) {
	run(GetParam(), "mini-apps/annotations/gp-annotations.exe", 0, 0);
}
TEST_P(FlagMode, DisabledAnnotations) {
	run(GetParam(), "mini-apps/annotations/gp-annotations-racy.exe", 1, 5);
}

// Individual tests

TEST_F(DrIntegration, ExclStack) {
	run("--excl-stack", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
}

TEST_F(DrIntegration, ExcludeRaces) {
	run("-c test/data/drace_excl.ini", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 0, 0);
}

#ifdef XML_EXPORTER
TEST_F(DrIntegration, XmlOutput) {
    std::string xmlfile("xmlOutput.xml");
    run("--xml-file " + xmlfile, "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
    {
        tinyxml2::XMLDocument doc;
        ASSERT_EQ(doc.LoadFile(xmlfile.c_str()), tinyxml2::XML_SUCCESS) << "File not found";
        const auto errornode = doc.FirstChildElement("valgrindoutput")->FirstChildElement("error");
        EXPECT_GT(errornode->FirstChildElement("tid")->IntText(), 0);
        EXPECT_STREQ(errornode->FirstChildElement("kind")->GetText(), "Race");
    }
    std::remove(xmlfile.c_str());
}
#endif

// Setup value-parameterized tests
INSTANTIATE_TEST_CASE_P(DrIntegration,
	FlagMode,
	::testing::Values("--sync-mode", ""));
