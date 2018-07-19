#include "gtest/gtest.h"
#include "detectorTest.h"

TEST_F(DetectorTest, WR_Race) {
	detector::fork(1, 10);
	detector::fork(1, 11);
	detector::write(10, (void*) 0x0010,(void*)0x0010, 8);
	detector::read(11,  (void*)0x0011, (void*)0x0010, 8);

	EXPECT_EQ(num_races, 1);
}

TEST_F(DetectorTest, Mutex) {
	detector::fork(1, 20);
	detector::fork(1, 21);
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
	detector::fork(1, 30);
	detector::write(30, (void*)0x0031, (void*)0x0032, 8);

	detector::fork(1, 31);
	detector::write(31, (void*)0x0032, (void*)0x0032, 8);
	detector::join(1, 31);

	detector::read(30, (void*)0x0033, (void*)0x0032, 8);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, MultiFork) {
	detector::fork(1, 40);
	detector::fork(1, 41);
	detector::fork(1, 42);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, HappensBefore) {
	detector::write(50, (void*)0x0050, (void*)0x0050, 8);
	detector::happens_before(50, (void*)5051);
	detector::happens_after(51, (void*)5051);
	detector::write(51, (void*)0x0051, (void*)0x0050, 8);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, ForkInitialize) {
	detector::fork(1, 60);
	detector::write(60, (void*)0x0060, (void*)0x0060, 8);
	detector::fork(1, 61);
	detector::write(61, (void*)0x0060, (void*)0x0060, 8);

	EXPECT_EQ(num_races, 0);
}
