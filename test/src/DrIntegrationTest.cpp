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

#include "DrIntegrationTest.h"

#include <string>

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
//TODO: check why this test hangs/deadlocks sometimes
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

// Setup value-parameterized tests
INSTANTIATE_TEST_CASE_P(DrIntegration,
	FlagMode,
	::testing::Values("--sync-mode", ""));
