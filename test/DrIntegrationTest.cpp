#include "DrIntegrationTest.h"

#include <string>

std::string DrIntegrationTest::drrun = "drrun.exe";

TEST_F(DrIntegrationTest, DefaultParams) {
	run("", "mini-apps/concurrent-inc/gp-concurrent-inc.exe", 1, 10);
	run("", "mini-apps/inc-mutex/gp-inc-mutex.exe", 0, 0);
	run("", "mini-apps/lock-kinds/gp-lock-kinds.exe", 0, 0);
	run("", "mini-apps/empty-main/gp-empty-main.exe", 0, 0);
}