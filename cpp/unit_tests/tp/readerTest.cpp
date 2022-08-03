/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "readerTest.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace toroni::tp::unit_tests;

TEST_F(ReaderTest, FirstLastReader) {
  MockChannelHandler mockChannelHandler;

  InSequence s;
  EXPECT_CALL(mockEventCb, Call(toroni::tp::FIRST_CHANNEL_READER_CREATED))
      .Times(1);
  EXPECT_CALL(mockEventCb, Call(toroni::tp::LAST_CHANNEL_READER_CLOSED))
      .Times(1);

  toroni::Ref<toroni::tp::ChannelReader> channelReader =
      reader->CreateChannelReader("ch", mockChannelHandler.AsStdFunction(),
                                  true);
  RunQueue(sqReader);
  EXPECT_TRUE(readerInfo->rmpReaderInfo.Get(0).isActive); // use slot at index 0

  reader->CloseChannelReader(channelReader.get());
  RunQueue(sqReader);
  EXPECT_FALSE(readerInfo->rmpReaderInfo.Get(0).isActive);
}

MATCHER_P(StrEqVoidPointer, expected, "") {
  return std::string(static_cast<const char *>(arg)) == expected;
}

TEST_F(ReaderTest, TopicMatch) {
  EXPECT_CALL(mockEventCb, Call(_)).Times(AnyNumber());

  // channel closed, not called
  MockChannelHandler h1;
  EXPECT_CALL(h1, Call(_, _)).Times(0);
  toroni::Ref<toroni::tp::ChannelReader> channelReader =
      reader->CreateChannelReader("ch", h1.AsStdFunction(), true);
  reader->CloseChannelReader(channelReader.get());

  // channel matching
  MockChannelHandler h2;
  EXPECT_CALL(h2, Call(StrEqVoidPointer("data"), 4));
  reader->CreateChannelReader("ch", h2.AsStdFunction(), false);

  // channel not matching
  MockChannelHandler h3;
  EXPECT_CALL(h3, Call(_, _)).Times(0);
  reader->CreateChannelReader("h", h3.AsStdFunction(), false);

  // channel matching, handle descendants
  MockChannelHandler h5;
  EXPECT_CALL(h5, Call(_, _)).Times(1);
  reader->CreateChannelReader("c", h5.AsStdFunction(), true);

  RunQueue(sqReader); // register all readers
  PostMessage("ch", "data", true);

  // channel created after message was posted, not receiving
  MockChannelHandler h4;
  EXPECT_CALL(h4, Call(_, _)).Times(0);
  reader->CreateChannelReader("h4", h4.AsStdFunction(), false);

  reader->Run();
  RunQueue(sqReader); // get readers
  RunQueue(sqRmp);    // read from rmp
}

TEST_F(ReaderTest, Expired) {
  EXPECT_CALL(mockEventCb, Call(_)).Times(AnyNumber());
  EXPECT_CALL(mockEventCb, Call(toroni::tp::ALL_CHANNEL_READERS_EXPIRED))
      .Times(1);

  // call back not invoked
  MockChannelHandler h2;
  EXPECT_CALL(h2, Call(_, _)).Times(0);
  reader->CreateChannelReader("ch", h2.AsStdFunction(), false);

  RunQueue(sqReader); // create readers

  WriteBigData(noBPHandler);
  WriteBigData(noBPHandler);

  reader->Run();
  RunQueue(sqReader); // get readers
  RunQueue(sqRmp);    // read from rmp
}

TEST_F(ReaderTest, RunWithoutReaders) {
  reader->Run();

  EXPECT_EQ(sqReader.size(), 1);
  EXPECT_EQ(sqRmp.size(), 0);
}