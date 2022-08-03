/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "toroni/tp/asyncWriter.hpp"
#include "toroni/tp/readerInfo.hpp"

#include "asyncWriterTest.hpp"

using namespace toroni::tp;
using namespace toroni::tp::unit_tests;

TEST_F(AsyncWriterTest, PostEnqueuesWhenToldTo) {
  toroni::Ref<AsyncWriter> writer =
      AsyncWriter::Create(ringBuf, readerInfo, enqMsgFn.AsStdFunction(),
                          drainMsgFn.AsStdFunction(), enqWorkFn.AsStdFunction(),
                          bpFn.AsStdFunction(), notifyFn.AsStdFunction());
  TopicMsgBinaryPtr msg = writer->CreateMessage("1", "1", 1, true);

  InSequence s;
  EXPECT_CALL(enqMsgFn, Call(_)).WillOnce(Return(true));
  EXPECT_CALL(enqWorkFn, Call(_)).Times(1);
  EXPECT_CALL(enqMsgFn, Call(_)).WillOnce(Return(true));
  EXPECT_CALL(enqWorkFn, Call(_)).Times(1);
  EXPECT_CALL(enqMsgFn, Call(_)).WillOnce(Return(false));
  EXPECT_CALL(enqWorkFn, Call(_)).Times(0);
  EXPECT_CALL(notifyFn, Call()).Times(0);
  EXPECT_CALL(drainMsgFn, Call()).Times(0);
  EXPECT_CALL(bpFn, Call(_, _)).Times(0);

  writer->Post(msg);
  writer->Post(msg);
  writer->Post(msg);
}

TEST_F(AsyncWriterTest, PostDrainNotify) {
  toroni::Ref<AsyncWriter> writer = AsyncWriter::Create(
      ringBuf, readerInfo, enqMsgFn.AsStdFunction(), drainMsgFn.AsStdFunction(),
      [](auto fn) { fn(); }, bpFn.AsStdFunction(), notifyFn.AsStdFunction());
  TopicMsgBinaryPtr msg = writer->CreateMessage("1", "1", 1, true);

  InSequence s;
  vector<TopicMsgBinaryPtr> emptyQueue;
  vector<TopicMsgBinaryPtr> queueOneEl{msg};
  EXPECT_CALL(enqMsgFn, Call(_)).WillOnce(Return(true));
  EXPECT_CALL(drainMsgFn, Call()).WillOnce(Return(queueOneEl));
  EXPECT_CALL(drainMsgFn, Call()).WillOnce(Return(emptyQueue));
  EXPECT_CALL(notifyFn, Call()).Times(1);
  EXPECT_CALL(bpFn, Call(_, _)).Times(0);

  writer->Post(msg);

  EXPECT_NE(ringBuf->freePos, 0); // something written
}