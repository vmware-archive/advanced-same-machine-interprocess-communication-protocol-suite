/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "readerInfoTest.hpp"
#include <thread>

using namespace toroni::rmp;
using namespace toroni::rmp::unit_tests;
using namespace std;

TEST_F(ReaderInfoTest, Initialization) {
  EXPECT_TRUE(readerInfo->initialized);
  EXPECT_EQ(readerInfo->stats.expiredReaders, 0);

  // empty range
  uint16_t min, max;
  readerInfo->GetActiveRange(min, max);
  EXPECT_GE(min, max);
}

TEST_F(ReaderInfoTest, ActivateDeactivateRange) {
  uint16_t min, max;

  for (int i = 0; i < readerSlots; i++) {
    readerInfo->Activate(i, i + 1);
    readerInfo->GetActiveRange(min, max);
    EXPECT_TRUE(readerInfo->Get(i).isActive);
    EXPECT_EQ(readerInfo->Get(i).position, i + 1);
    EXPECT_EQ(min, 0);
    EXPECT_EQ(max, i + 1);
  }

  readerInfo->Deactivate(1);
  readerInfo->GetActiveRange(min, max);
  EXPECT_EQ(min, 0);
  EXPECT_EQ(max, 3);

  readerInfo->Deactivate(0);
  readerInfo->GetActiveRange(min, max);
  EXPECT_EQ(min, 2);
  EXPECT_EQ(max, 3);

  readerInfo->Deactivate(2);
  readerInfo->GetActiveRange(min, max);
  EXPECT_GE(min, max);
}

TEST_F(ReaderInfoTest, AllocFreeNoFree) {
  const int readers = 5;

  atomic<int> allocSuccess;
  atomic<int> allocFail;
  atomic<int> allocCount;
  allocCount = allocSuccess = allocFail = 0;

  auto proc = [&]() {
    auto readerId = readerInfo->Alloc();

    allocCount++;
    while (allocCount != readers)
      ;

    if (readerId == -1) {
      allocFail++;
      return;
    }

    allocSuccess++;

    readerInfo->Free(readerId);
  };

  vector<thread> threads;
  for (int i = 0; i < readers; i++) {
    threads.emplace_back(proc);
  }
  for (auto &t : threads) {
    t.join();
  }

  // 3 allocs succeeded, 2 failed
  EXPECT_EQ(allocSuccess.load(), readerSlots);
  EXPECT_EQ(allocFail.load(), readers - readerSlots);

  // empty range
  uint16_t min, max;
  readerInfo->GetActiveRange(min, max);
  EXPECT_GE(min, max);

  // can alloc again
  auto readerId = readerInfo->Alloc();
  EXPECT_EQ(readerId, 0);
}

TEST_F(ReaderInfoTest, AllocDead) {
  auto proc = [&]() {
    auto readerId = readerInfo->Alloc();
    readerInfo->Activate(readerId, 0);

    EXPECT_NE(readerId, ReaderInfo::INVALID_READER_ID);
    EXPECT_TRUE(readerInfo->Get(readerId).isActive);
  };

  // reuse readerSlots 3 times
  for (int i = 0; i < 3 * readerSlots; i++) {
    thread t(proc);
    t.join();
  }
}

class SingleReaderInfoTest : public ReaderInfoTest {
public:
  SingleReaderInfoTest() : ReaderInfoTest(1) {}
};

TEST_F(SingleReaderInfoTest, Alloc) {
  auto readerId = readerInfo->Alloc();
  readerInfo->Activate(readerId, 0);

  EXPECT_NE(readerId, ReaderInfo::INVALID_READER_ID);
}

class NoReaderInfoTest : public ReaderInfoTest {
public:
  NoReaderInfoTest() : ReaderInfoTest(0) {}
};

TEST_F(NoReaderInfoTest, Alloc) {
  auto readerId = readerInfo->Alloc();
  readerInfo->Activate(readerId, 0);

  EXPECT_EQ(readerId, ReaderInfo::INVALID_READER_ID);
}