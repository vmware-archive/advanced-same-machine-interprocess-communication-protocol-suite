/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASYNCWRITERTEST_HPP
#define ASYNCWRITERTEST_HPP

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "toroni/tp/asyncWriter.hpp"
#include "toroni/tp/readerInfo.hpp"

#include "rmp/byteRingBufferTest.hpp"
#include "rmp/readerInfoTest.hpp"

#include <gmock/gmock.h>

#include <new>
#include <vector>

namespace toroni {
namespace tp {
namespace unit_tests {

using namespace ::testing;
using namespace std;

class AsyncWriterTest : virtual public rmp::unit_tests::ByteRingBufferTest,
                        virtual public rmp::unit_tests::ReaderInfoTest {
protected:
  AsyncWriterTest() : AsyncWriterTest(3) {}
  AsyncWriterTest(int readerInfoSlots)
      : readerInfoMem(ReaderInfo::Size(readerInfoSlots)),
        readerInfo(new (readerInfoMem.data()) ReaderInfo(readerInfoSlots)) {}

  std::vector<char> readerInfoMem;
  ReaderInfo *const readerInfo;

  using MockDrainMsgFn = MockFunction<vector<TopicMsgBinaryPtr>()>;
  using MockEnqueueMsgFn = MockFunction<bool(const TopicMsgBinaryPtr &)>;
  using MockEnqueueWorkFn = MockFunction<void(const AsyncWriter::WorkFn &)>;
  using MockNotifyFn = MockFunction<void()>;
  using MockBackpressureHandlerFn =
      MockFunction<bool(rmp::Position, rmp::Position)>;

  MockEnqueueMsgFn enqMsgFn;
  MockDrainMsgFn drainMsgFn;
  MockEnqueueWorkFn enqWorkFn;
  MockNotifyFn notifyFn;
  MockBackpressureHandlerFn bpFn;

  void PostMessage(const string &topic, const string &data,
                   bool postToDescendants) {
    Ref<AsyncWriter> writer = AsyncWriter::Create(
        ringBuf, readerInfo, enqMsgFn.AsStdFunction(),
        drainMsgFn.AsStdFunction(), [](auto fn) { fn(); }, bpFn.AsStdFunction(),
        notifyFn.AsStdFunction());
    TopicMsgBinaryPtr msg = writer->CreateMessage(
        topic.c_str(), data.data(), data.size(), postToDescendants);

    InSequence s;
    vector<TopicMsgBinaryPtr> emptyQueue;
    vector<TopicMsgBinaryPtr> queueOneEl{msg};
    EXPECT_CALL(enqMsgFn, Call(_)).WillOnce(Return(true));
    EXPECT_CALL(drainMsgFn, Call()).WillOnce(Return(queueOneEl));
    EXPECT_CALL(drainMsgFn, Call()).WillOnce(Return(emptyQueue));
    EXPECT_CALL(notifyFn, Call()).Times(1);
    EXPECT_CALL(bpFn, Call(_, _)).Times(0);

    writer->Post(msg);
  }
};

} // namespace unit_tests
} // namespace tp
} // namespace toroni

#endif // ASYNCWRITERTEST_HPP
