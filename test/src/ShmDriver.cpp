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

#include "gtest/gtest.h"

#include "ipc/SyncSHMDriver.h"

TEST(SyncShmDriver, InitFinalize) {
	ipc::SyncSHMDriver<true, false> sender("test-shm", true);
	ipc::SyncSHMDriver<true, false> receiver("test-shm", true);

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

	ipc::SyncSHMDriver<true, false> sender("test-shm", true);
	ipc::SyncSHMDriver<true, false> receiver("test-shm", true);

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