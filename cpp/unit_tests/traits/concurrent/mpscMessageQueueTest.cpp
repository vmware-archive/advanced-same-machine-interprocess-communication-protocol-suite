/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/concurrent/mpscMessageQueue.hpp" // for MPSCMessage...

#include <gmock/gmock-matchers.h>      // for ElementsAre
#include <gmock/gmock-more-matchers.h> // for IsEmpty
#include <gtest/gtest-message.h>       // for Message
#include <gtest/gtest-test-part.h>     // for TestPartResult
#include <gtest/gtest_pred_impl.h>     // for EXPECT_EQ
#include <iosfwd>                      // for std

using namespace toroni::traits::concurrent;
using namespace ::testing;

struct MPSCMessageQueueTest : public ::testing::Test {
  MPSCMessageQueue<int> mq;
};

TEST_F(MPSCMessageQueueTest, EnqueueSingleDrainer) {
  EXPECT_EQ(mq.Enqueue(1), true);
  EXPECT_EQ(mq.Enqueue(1), false); // drainer already started
  EXPECT_EQ(mq.Enqueue(1), false);
}

TEST_F(MPSCMessageQueueTest, DrainEmpty) {
  EXPECT_THAT(mq.Drain(), IsEmpty());
  EXPECT_THAT(mq.Drain(), IsEmpty());
}

TEST_F(MPSCMessageQueueTest, EnqueueDrainControlAndData) {
  EXPECT_EQ(mq.Enqueue(1), true);
  EXPECT_EQ(mq.Enqueue(2), false);            // already started
  EXPECT_THAT(mq.Drain(), ElementsAre(1, 2)); // verify data
  EXPECT_EQ(mq.Enqueue(3), false);            // running
  EXPECT_THAT(mq.Drain(), ElementsAre(3));    // more data
  EXPECT_THAT(mq.Drain(), IsEmpty());         // empty, should start next time
  EXPECT_EQ(mq.Enqueue(4), true);
}
