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

#include "IntegrationTest.h"

#include <iostream>
#include <string>
#include <future>

#ifdef DRACE_XML_EXPORTER
#include <tinyxml2.h>
#include <cstdio>
#endif

bool Integration::verbose = false;

// Test cases

TEST_P(DR, ConcurrentInc) {
	run(GetParam(), "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
}
TEST_P(DR, IncMutex) {
	run(GetParam(), "mini-apps/inc-mutex/gp-inc-mutex.exe", 0, 0);
}
TEST_P(DR, LockKinds) {
	run(GetParam(), "mini-apps/lock-kinds/gp-lock-kinds.exe", 0, 0);
}
TEST_P(DR, EmptyMain) {
	run(GetParam(), "mini-apps/empty-main/gp-empty-main.exe", 0, 0);
}
TEST_P(DR, Atomics) {
	run(GetParam(), "mini-apps/atomics/gp-atomics.exe", 0, 0);
}
// TODO: Redesign test:
//       Currently, many runs miss the race as the race often
//       does not occur.
//TEST_P(FlagMode, RacyAtomics) {
//	run(GetParam(), "mini-apps/atomics/gp-atomics.exe racy", 1, 10);
//}
TEST_P(DR, Annotations) {
	run(GetParam(), "mini-apps/annotations/gp-annotations.exe", 0, 0);
}
TEST_P(DR, DisabledAnnotations) {
	run(GetParam(), "mini-apps/annotations/gp-annotations-racy.exe", 1, 5);
}

// Individual tests

TEST_P(DR, DelayedLookup) {
    // with delayed lookup all races have to be cached,
    // hence make test more difficult by disabling suppressions
    run(std::string(GetParam()) + " --delay-syms --suplevel 0", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 210);
}

TEST_P(DR, ExclStack) {
	run(std::string(GetParam()) + " --excl-stack", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
}

TEST_P(DR, ExcludeRaces) {
	run(std::string(GetParam()) + " -c test/data/drace_excl.ini", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 0, 0);
}

#ifdef DRACE_TESTING_DOTNET
// Dotnet Tests
TEST_P(DR, DotnetClrRacy) {
    // TODO: start msr
    auto msr_task = std::thread([]() {std::system("ManagedResolver\\msr.exe --once"); });
    run(std::string(GetParam()) + " --extctrl", "mini-apps/cs-sync/gp-cs-sync-clr.exe none", 1, 10);
    msr_task.join();
}

TEST_P(DR, DotnetClrMonitor) {
    // TODO: start msr
    auto msr_task = std::thread([]() {std::system("ManagedResolver\\msr.exe --once"); });
    run(std::string(GetParam()) + " --extctrl", "mini-apps/cs-sync/gp-cs-sync-clr.exe monitor", 0, 0);
    msr_task.join();
}

TEST_P(DR, DotnetClrMutex) {
    // TODO: start msr
    auto msr_task = std::thread([]() {std::system("ManagedResolver\\msr.exe --once"); });
    run(std::string(GetParam()) + " --extctrl", "mini-apps/cs-sync/gp-cs-sync-clr.exe mutex", 0, 0);
    msr_task.join();
}
#endif

#ifdef DRACE_XML_EXPORTER
TEST_P(DR, ReportXML) {
    std::string filename("reportXML.xml");
    run(std::string(GetParam()) + " --xml-file " + filename, "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
    {
        tinyxml2::XMLDocument doc;
        ASSERT_EQ(doc.LoadFile(filename.c_str()), tinyxml2::XML_SUCCESS) << "File not found";
        const auto errornode = doc.FirstChildElement("valgrindoutput")->FirstChildElement("error");
        EXPECT_GT(errornode->FirstChildElement("tid")->UnsignedText(), 0u);
        EXPECT_STREQ(errornode->FirstChildElement("kind")->GetText(), "Race");

        const auto status = doc.FirstChildElement("valgrindoutput")->LastChildElement("status");
        EXPECT_STREQ(status->FirstChildElement("state")->GetText(), "FINISHED");
        EXPECT_GT(status->FirstChildElement("duration")->UnsignedText(), 0u);
        EXPECT_GT(errornode->FirstChildElement("timestamp")->UnsignedText(), 0u);
        EXPECT_EQ(errornode->FirstChildElement("stack")->FirstChildElement("frame")->FirstChildElement("offset")->UnsignedText(), 0u);

    }
    std::remove(filename.c_str());
}
#endif

TEST_P(DR, ReportText) {
    std::string filename("reportText.txt");
    run(std::string(GetParam()) + " --out-file " + filename, "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
    {
        std::ifstream fstr(filename);
        if (fstr.good()) {
            std::string data((std::istreambuf_iterator<char>(fstr)), (std::istreambuf_iterator<char>()));
            EXPECT_FALSE(data.find("Access 1 tid") == std::string::npos) << "race not found in report";
        }
        else {
            ADD_FAILURE() << "file not found";
        }
    }
    std::remove(filename.c_str());
}

// Setup value-parameterized tests
INSTANTIATE_TEST_CASE_P(Integration,
    DR,
	::testing::Values("-d fasttrack", "-d tsan"));
