#include "DrIntegrationTest.h"

#include <string>

std::string DrIntegrationTest::drrun = "drrun.exe";

TEST_F(DrIntegrationTest, DefaultParams) {
	run("", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
	run("", "mini-apps/inc-mutex/gp-inc-mutex.exe", 0, 0);
	run("", "mini-apps/lock-kinds/gp-lock-kinds.exe", 0, 0);
	run("", "mini-apps/empty-main/gp-empty-main.exe", 0, 0);
}

TEST_F(DrIntegrationTest, FastMode) {
	std::string flags = "--fast-mode";
	run(flags, "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
	run(flags, "mini-apps/inc-mutex/gp-inc-mutex.exe", 0, 0);
	run(flags, "mini-apps/lock-kinds/gp-lock-kinds.exe", 0, 0);
	run(flags, "mini-apps/empty-main/gp-empty-main.exe", 0, 0);
}

TEST_F(DrIntegrationTest, ExcludeRaces) {
	run("-c test/data/drace_excl.ini", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 0, 0);
}