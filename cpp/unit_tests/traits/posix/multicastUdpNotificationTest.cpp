/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/posix/multicastUdpNotification.hpp"

#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>

using namespace toroni::traits;

struct MulticastUdpNotificationTest : public ::testing::Test {
  MulticastUdpNotification un{"226.1.1.1", 3334, "127.0.0.1"};
};

TEST_F(MulticastUdpNotificationTest, testNotifyPeekWaitPeek) {
  EXPECT_EQ(un.Peek(), false);
  EXPECT_NO_THROW(un.Send());
  EXPECT_EQ(un.Peek(), true);
  EXPECT_EQ(un.Peek(), true);
  EXPECT_NO_THROW(un.Wait());
  EXPECT_EQ(un.Peek(), false);
  EXPECT_EQ(un.Peek(), false);
}

TEST_F(MulticastUdpNotificationTest, testMatchNotifications) {
  EXPECT_NO_THROW(un.Send());
  EXPECT_NO_THROW(un.Send());
  EXPECT_NO_THROW(un.Wait());
  EXPECT_NO_THROW(un.Wait());
}

TEST_F(MulticastUdpNotificationTest, testWaitBlocks) {
  for (int i = 0; i < 5; i++) {
    std::future<void> future =
        std::async(std::launch::async, [&]() { un.Wait(); });

    // Blocks without send
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(5)),
              std::future_status::timeout);

    un.Send();

    // Unblocks after send
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(5)),
              std::future_status::ready);
  }
}