/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BYTERINGBUFFERTEST_HPP
#define BYTERINGBUFFERTEST_HPP

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "toroni/rmp/byteRingBuffer.hpp"

#include <gtest/gtest.h>

#include <new>
#include <vector>

namespace toroni {
namespace rmp {
namespace unit_tests {

class ByteRingBufferTest : virtual public ::testing::Test {
protected:
  ByteRingBufferTest() : ByteRingBufferTest(1024) {}

  ByteRingBufferTest(int bufSizeBytes)
      : ringBufSizeBytes(bufSizeBytes),
        ringBufMem(toroni::rmp::ByteRingBuffer::Size(bufSizeBytes)),
        ringBuf(new (ringBufMem.data())
                    toroni::rmp::ByteRingBuffer(bufSizeBytes)) {}

  size_t ringBufSizeBytes;
  std::vector<char> ringBufMem;
  toroni::rmp::ByteRingBuffer *const ringBuf;
};

} // namespace unit_tests
} // namespace rmp
} // namespace toroni

#endif // BYTERINGBUFFERTEST_HPP
