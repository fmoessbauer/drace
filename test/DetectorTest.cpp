#include "gtest/gtest.h"
#include "detectorTest.h"

TEST_F(DetectorTest, WR_Race) {
	detector::write(10, (void*) 0x0010,(void*)0x0010, 8);
	detector::read(11,  (void*)0x0011, (void*)0x0010, 8);

	EXPECT_EQ(num_races, 2);
}

TEST_F(DetectorTest, Mutex) {
	// First Thread
	detector::acquire(20, (void*)0x0100);
	detector::write(20,   (void*)0x0021, (void*)0x0020, 8);
	detector::read(20,    (void*)0x0022, (void*)0x0020, 8);
	detector::release(20, (void*)0x0100);

	// Second Thread
	detector::acquire(21, (void*)0x0100);
	detector::read(21,    (void*)0x0024, (void*)0x0020, 8);
	detector::write(21,   (void*)0x0025, (void*)0x0020, 8);
	detector::release(21, (void*)0x0100);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, ThreadExit) {
	detector::write(30, (void*)0x0031, (void*)0x0030, 8);

	detector::fork(1, 31);
	detector::write(31, (void*)0x0032, (void*)0x0030, 8);
	detector::join(1, 31);

	detector::read(30, (void*)0x0032, (void*)0x0030, 8);

	EXPECT_EQ(num_races, 0);
}
