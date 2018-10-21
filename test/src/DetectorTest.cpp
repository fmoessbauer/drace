#include "gtest/gtest.h"
#include "detectorTest.h"

TEST_F(DetectorTest, WR_Race) {
	detector::tls_t tls10;
	detector::tls_t tls11;

	detector::fork(1, 10, &tls10);
	detector::fork(1, 11, &tls11);

	stack[0] = 0x0010;
	detector::write(10, stack,1, (void*)0x0010, 8, tls10);
	stack[0] = 0x0011;
	detector::read(11,  stack,1, (void*)0x0010, 8, tls11);

	EXPECT_EQ(num_races, 1);
}

TEST_F(DetectorTest, Mutex) {
	detector::tls_t tls20;
	detector::tls_t tls21;

	detector::fork(1, 20, &tls20);
	detector::fork(1, 21, &tls21);
	// First Thread
	detector::acquire(20, (void*)0x0100, 1, true, false, tls20);
	stack[0] = 0x0021;
	detector::write(20, stack,1, (void*)0x0020, 8, tls20);
	stack[0] = 0x0022;
	detector::read(20, stack,1, (void*)0x0020, 8, tls20);
	detector::release(20, (void*)0x0100, 1, true, tls20);

	// Second Thread
	detector::acquire(21, (void*)0x0100, 1, true, false, tls21);
	stack[0] = 0x0024;
	detector::read(21,  stack,1, (void*)0x0020, 8, tls21);
	stack[0] = 0x0025;
	detector::write(21, stack,1, (void*)0x0020, 8, tls21);
	detector::release(21, (void*)0x0100, 1, true, tls21);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, ThreadExit) {
	detector::tls_t tls30;
	detector::tls_t tls31;

	detector::fork(1, 30, &tls30);
	stack[0] = 0x0031;
	detector::write(30, stack, 1, (void*)0x0032, 8, tls30);

	detector::fork(1, 31, &tls31);
	stack[0] = 0x0032;
	detector::write(31, stack, 1, (void*)0x0032, 8, tls31);
	detector::join(1, 31);

	stack[0] = 0x0031;
	detector::read(30, stack,1, (void*)0x0032, 8, tls30);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, MultiFork) {
	detector::tls_t tls40;
	detector::tls_t tls41;
	detector::tls_t tls42;

	detector::fork(1, 40, &tls40);
	detector::fork(1, 41, &tls41);
	detector::fork(1, 42, &tls42);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, HappensBefore) {
	detector::tls_t tls50;
	detector::tls_t tls51;

	detector::fork(1, 50, &tls50);
	detector::fork(1, 51, &tls51);

	stack[0] = 0x0050;
	detector::write(50, stack, 1, (void*)0x0050, 8, tls50);
	detector::happens_before(50, (void*)5051);
	detector::happens_after(51, (void*)5051);
	stack[0] = 0x0051;
	detector::write(51, stack,1, (void*)0x0050, 8, tls51);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, ForkInitialize) {
	detector::tls_t tls60;
	detector::tls_t tls61;

	detector::fork(1, 60, &tls60);
	stack[0] = 0x0060;
	detector::write(60, stack,1, (void*)0x0060, 8, tls60);
	detector::fork(1, 61, &tls61);
	detector::write(61, stack,1, (void*)0x0060, 8, tls61);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, Barrier) {
	detector::tls_t tls70;
	detector::tls_t tls71;
	detector::tls_t tls72;

	detector::fork(1, 70, &tls70);
	detector::fork(1, 71, &tls71);
	detector::fork(1, 72, &tls72);

	stack[0] = 0x0070;
	detector::write(70, stack, 1, (void*)0x0070, 8, tls70);
	detector::write(70, stack, 1, (void*)0x0071, 8, tls70);
	detector::write(71, stack, 1, (void*)0x0171, 8, tls71);

	// Barrier
	{
		void* barrier_id = (void*)0x0700;
		// barrier enter
		detector::happens_before(70, barrier_id);
		// each thread enters individually
		detector::write(71, stack, 1, (void*)0x0072, 8, tls71);
		detector::happens_before(71, barrier_id);

		// sufficient threads have arrived => barrier leave
		detector::happens_after(71, barrier_id);
		detector::write(71, stack, 1, (void*)0x0071, 8, tls71);
		detector::happens_after(70, barrier_id);
	}

	stack[0] = 0x0071;
	detector::write(71, stack, 1, (void*)0x0070, 8, tls71);
	stack[0] = 0x0072;
	detector::write(70, stack, 1, (void*)0x0072, 8, tls70);

	EXPECT_EQ(num_races, 0);
	// This thread did not paricipate in barrier, hence expect race
	detector::write(72, stack, 1, (void*)0x0071, 8, tls72);
	EXPECT_EQ(num_races, 1);
}
