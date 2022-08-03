/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/posix/robustProcMutex.hpp"
#include "toroni/exclusiveLock.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

using namespace toroni;
using namespace toroni::traits;
using namespace std;

TEST(RobustProcMutex, TryLockUnlocked) {
  RobustProcMutex mtx;

  thread t([&]() { ExclusiveLock<RobustProcMutex> lk(mtx); });
  t.join();

  EXPECT_EQ(mtx.TryLock(), true);
}

TEST(RobustProcMutex, TryLockDead) {
  RobustProcMutex mtx;

  thread t([&]() { mtx.Lock(); });
  t.join();

  EXPECT_EQ(mtx.TryLock(), true);
}

TEST(RobustProcMutex, TryLockLocked) {
  RobustProcMutex mtx;

  atomic<bool> locked{false}, finish{false};
  thread t([&]() {
    mtx.Lock();
    locked = true;
    while (!finish)
      ;
  });

  while (!locked)
    ;
  EXPECT_EQ(mtx.TryLock(), false);

  finish = true;
  t.join();
}

TEST(RobustProcMutex, LockLocked) {
  RobustProcMutex mtx;

  atomic<bool> locked1{false}, locked2{false}, finish1{false};
  thread t1([&]() {
    mtx.Lock();
    locked1 = true;
    while (!finish1)
      ;
  });

  while (!locked1)
    ;

  thread t2([&]() {
    mtx.Lock();
    locked2 = true;
  });

  this_thread::sleep_for(1ms);
  EXPECT_EQ(locked2, false);

  finish1 = true; // t1 exits/dies

  this_thread::sleep_for(1ms);
  EXPECT_EQ(locked2, true); // t2 can acquire

  t1.join();
  t2.join();
}
