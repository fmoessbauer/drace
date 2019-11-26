/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Philip Harr <philip.harr@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "gtest/gtest.h"
#include "fasttrack_test.h"
//#include "stacktrace.h"


using ::testing::UnitTest;

TEST(FasttrackTest, Test1) {
	ASSERT_EQ(1,1);
}

TEST(FasttrackTest, basic_stacktrace) {
	StackTrace st;

	st.push_stack_element(1);
	st.set_read_write(100, 1000);
	st.set_read_write(101, 1001);
	
	st.push_stack_element(2);
	st.set_read_write(102, 1002);
	
	st.push_stack_element(3);
	st.set_read_write(103, 1004);
	
	st.pop_stack_element();

	st.push_stack_element(1);
	st.push_stack_element(2);
	st.push_stack_element(6);
	st.set_read_write(104, 1004);
	st.push_stack_element(7);
	
	std::list<size_t> list = st.return_stack_trace(104);
	std::vector<size_t> vec(list.begin(), list.end());
	
	ASSERT_EQ(vec[0], 1);
	ASSERT_EQ(vec[1], 2);
	ASSERT_EQ(vec[2], 1);
	ASSERT_EQ(vec[3], 2);
	ASSERT_EQ(vec[4], 6);
	ASSERT_EQ(vec[5], 1004);
	ASSERT_EQ(vec.size(), 6);
}


