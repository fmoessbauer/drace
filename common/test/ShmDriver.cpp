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

#include <random>
#include <string>
#include "gtest/gtest.h"
#include "ipc/SyncSHMDriver.h"

/**
 * \note use unique names for SHM segments as tests
 *       might run in parallel
 */

/**
 * \brief Return the name with a unique suffix
 */
std::string getUniqueName(const std::string & name){
	return name + std::to_string(std::random_device()());
}

TEST(SyncShmDriver, InitFinalize) {
	std::string shmseg(getUniqueName("test-shm-if"));
	ipc::SyncSHMDriver<true, false> sender(shmseg.c_str(), true);
	ipc::SyncSHMDriver<true, false> receiver(shmseg.c_str(), true);

	sender.id(ipc::SMDataID::CONNECT);
	sender.commit();

	receiver.wait_receive();
	ASSERT_EQ(receiver.id(), ipc::SMDataID::CONNECT);

	receiver.put<int>(ipc::SMDataID::ATTACH, 42);
	receiver.commit();

	sender.wait_receive();
	ASSERT_EQ(sender.id(), ipc::SMDataID::ATTACH);
	ASSERT_EQ(sender.get<int>(), 42);
}

TEST(SyncShmDriver, Emplace) {
	struct test_t {
		int a = 10;
		bool b = false;
	};
	std::string shmseg(getUniqueName("test-shm-emplace"));

	ipc::SyncSHMDriver<true, false> sender(shmseg.c_str(), true);
	ipc::SyncSHMDriver<true, false> receiver(shmseg.c_str(), true);

	sender.emplace<test_t>(ipc::SMDataID::SYMBOL);
	sender.commit();

	ASSERT_EQ(sender.get<test_t>().a, 10);
	ASSERT_FALSE(sender.get<test_t>().b);

	receiver.wait_receive();
	auto ret = receiver.get<test_t>();
	ASSERT_EQ(receiver.id(), ipc::SMDataID::SYMBOL);
	ASSERT_EQ(ret.a, 10);
	ASSERT_FALSE(ret.b);
}