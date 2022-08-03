/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef READERTEST_HPP
#define READERTEST_HPP

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "asyncWriterTest.hpp"
#include "rmp/readerWriterTest.hpp"
#include "toroni/tp/reader.hpp"

#include <functional>
#include <vector>

namespace toroni {
namespace tp {
namespace unit_tests {

class ReaderTest : public AsyncWriterTest,
                   public rmp::unit_tests::ReaderWriterTest {
protected:
  ReaderTest() {
    EnqueueSerialFn efn;
    ChannelReaderEventCb cb;
    reader = Reader::Create(
        ringBuf, readerInfo,
        [&](auto... param) { sqReader.emplace_back(param...); },
        [&](auto... param) { sqRmp.emplace_back(param...); },
        mockEventCb.AsStdFunction());
  }

  toroni::Ref<Reader> reader;
  MockFunction<void(ChannelReaderEventType)> mockEventCb;
  using MockChannelHandler = MockFunction<void(const void *, size_t)>;

  vector<std::function<void()>> sqReader, sqRmp;
  void RunQueue(vector<std::function<void()>> &queue) {
    for (auto &i : queue) {
      i();
    }
    queue.clear();
  }
};

} // namespace unit_tests
} // namespace tp
} // namespace toroni

#endif // READERTEST_HPP
