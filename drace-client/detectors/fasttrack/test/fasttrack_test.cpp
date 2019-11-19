#include "gtest/gtest.h"
#include "fasttrack_test.h"


using ::testing::UnitTest;

int    t_argc;
char** t_argv;

int main(int argc, char ** argv) {
	::testing::InitGoogleTest(&argc, argv);

	t_argc = argc;
	t_argv = argv;

	int ret = RUN_ALL_TESTS();
	return ret;
}



TEST(FasttrackTest, Test1) {
	ASSERT_EQ(1,3);
}