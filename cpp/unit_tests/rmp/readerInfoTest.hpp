/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef READERINFOTEST_HPP
#define READERINFOTEST_HPP

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "toroni/rmp/readerInfo.hpp"

#include <gtest/gtest.h>

#include <new>
#include <vector>

namespace toroni {
namespace rmp {
namespace unit_tests {

class ReaderInfoTest : virtual public ::testing::Test {

protected:
  ReaderInfoTest() : ReaderInfoTest(3) {}

  ReaderInfoTest(size_t slots)
      : readerSlots(slots), readerInfoMem(toroni::rmp::ReaderInfo::Size(slots)),
        readerInfo(new (readerInfoMem.data()) toroni::rmp::ReaderInfo(slots)) {}

  const int readerSlots;
  std::vector<char> readerInfoMem;
  toroni::rmp::ReaderInfo *const readerInfo;
};

} // namespace unit_tests
} // namespace rmp
} // namespace toroni

#endif // READERINFOTEST_HPP
