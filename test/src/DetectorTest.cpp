#include "gtest/gtest.h"
#include "detectorTest.h"

TEST_F(DetectorTest, WR_Race) {
	detector::tls_t tls10;
	detector::tls_t tls11;

	detector::fork(1, 10, &tls10);
	detector::fork(1, 11, &tls11);

	stack[0] = 0x0010;
	detector::write(tls10, stack,1, (void*)0x0010, 8);
	stack[0] = 0x0011;
	detector::read(tls11,  stack,1, (void*)0x0010, 8);

	EXPECT_EQ(num_races, 1);
}

TEST_F(DetectorTest, Mutex) {
	detector::tls_t tls20;
	detector::tls_t tls21;

	detector::fork(1, 20, &tls20);
	detector::fork(1, 21, &tls21);
	// First Thread
	detector::acquire(tls20, (void*)0x0100, 1, true);
	stack[0] = 0x0021;
	detector::write(tls20, stack,1, (void*)0x0020, 8);
	stack[0] = 0x0022;
	detector::read(tls20, stack,1, (void*)0x0020, 8);
	detector::release(tls20, (void*)0x0100, true);

	// Second Thread
	detector::acquire(tls21, (void*)0x0100, 1, true);
	stack[0] = 0x0024;
	detector::read(tls21,  stack,1, (void*)0x0020, 8);
	stack[0] = 0x0025;
	detector::write(tls21, stack,1, (void*)0x0020, 8);
	detector::release(tls21, (void*)0x0100, true);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, ThreadExit) {
	detector::tls_t tls30;
	detector::tls_t tls31;

	detector::fork(1, 30, &tls30);
	stack[0] = 0x0031;
	detector::write(tls30, stack, 1, (void*)0x0032, 8);

	detector::fork(1, 31, &tls31);
	stack[0] = 0x0032;
	detector::write(tls31, stack, 1, (void*)0x0032, 8);
	detector::join(1, 31, nullptr);

	stack[0] = 0x0031;
	detector::read(tls30, stack,1, (void*)0x0032, 8);

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
	detector::write(tls50, stack, 1, (void*)0x0050, 8);
	detector::happens_before(50, (void*)5051);
	detector::happens_after(51, (void*)5051);
	stack[0] = 0x0051;
	detector::write(tls51, stack,1, (void*)0x0050, 8);

	EXPECT_EQ(num_races, 0);
}

TEST_F(DetectorTest, ForkInitialize) {
	detector::tls_t tls60;
	detector::tls_t tls61;

	detector::fork(1, 60, &tls60);
	stack[0] = 0x0060;
	detector::write(tls60, stack,1, (void*)0x0060, 8);
	detector::fork(1, 61, &tls61);
	detector::write(tls61, stack,1, (void*)0x0060, 8);

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
	detector::write(tls70, stack, 1, (void*)0x0070, 8);
	detector::write(tls70, stack, 1, (void*)0x0071, 8);
	detector::write(tls71, stack, 1, (void*)0x0171, 8);

	// Barrier
	{
		void* barrier_id = (void*)0x0700;
		// barrier enter
		detector::happens_before(70, barrier_id);
		// each thread enters individually
		detector::write(tls71, stack, 1, (void*)0x0072, 8);
		detector::happens_before(71, barrier_id);

		// sufficient threads have arrived => barrier leave
		detector::happens_after(71, barrier_id);
		detector::write(tls71, stack, 1, (void*)0x0071, 8);
		detector::happens_after(70, barrier_id);
	}

	stack[0] = 0x0071;
	detector::write(tls71, stack, 1, (void*)0x0070, 8);
	stack[0] = 0x0072;
	detector::write(tls70, stack, 1, (void*)0x0072, 8);

	EXPECT_EQ(num_races, 0);
	// This thread did not paricipate in barrier, hence expect race
	detector::write(tls72, stack, 1, (void*)0x0071, 8);
	EXPECT_EQ(num_races, 1);
}
