/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/concurrent/serialWorkQueue.hpp" // for SerialWorkIt...
#include "toroni/ref.hpp"                               // for Ref, toroni

#include "gtest/gtest_pred_impl.h" // for Test, SuiteA...
#include <gmock/gmock-matchers.h>  // for ElementsAre
#include <gtest/gtest-message.h>   // for Message
#include <gtest/gtest-test-part.h> // for TestPartResult

#include <chrono> // for operator""ms
#include <iosfwd> // for std
#include <memory> // for __shared_ptr...
#include <thread> // for thread, slee...
#include <vector> // for vector

using namespace toroni;
using namespace std;
using namespace toroni::traits::concurrent;
using ::testing::ElementsAre;

TEST(SerialWorkQueue, OrderSingleProducer) {
  vector<int> vec;

  {
    Ref<SerialWorkItemQueue> wq = SerialWorkItemQueue::Start();

    for (int i = 0; i < 5; i++) {
      wq->Enqueue([=, &vec]() { vec.push_back(i); });
    }

    this_thread::sleep_for(1ms); // simulate queue waits

    for (int i = 5; i < 10; i++) {
      wq->Enqueue([=, &vec]() { vec.push_back(i); });
    }

    // wait for wq to drain
    wq->Stop(StopPolicy::IF_EMPTY);
  }

  ASSERT_THAT(vec, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(SerialWorkQueue, CountManyProducers) {
  vector<int> vec;
  vector<thread> pushers;

  {
    Ref<SerialWorkItemQueue> wq = SerialWorkItemQueue::Start();

    for (int i = 0; i < 25; i++) {
      pushers.emplace_back(
          [=, &vec]() { wq->Enqueue([=, &vec]() { vec.push_back(i); }); });
    }

    // wait for pushers
    for (auto &t : pushers) {
      t.join();
    }

    // wait for wq to drain
    wq->Stop(StopPolicy::IF_EMPTY);
  }

  EXPECT_EQ(vec.size(), 25);
}