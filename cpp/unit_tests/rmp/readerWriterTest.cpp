#include "readerWriterTest.hpp"

#include <gmock/gmock-more-matchers.h>
#include <thread>

using namespace toroni::rmp;
using namespace toroni::rmp::unit_tests;
using namespace std;
using namespace ::testing;

TEST_F(ReaderWriterTest, ScratchReaderBPReadEmpty) {
  readerBP.Activate();
  readerBP.ReadEx(readHandler);

  EXPECT_TRUE(readHandler.Data.empty());
}

TEST_F(ReaderWriterTest, ScratchWrite2Read2ReadEmpty) {
  readerBP.Activate();

  WriteInt(1);
  WriteInt(2);

  EXPECT_EQ(readerBP.ReadEx(readHandler), Reader::SUCCESS);
  EXPECT_THAT(readHandler.Data, ElementsAre(1, 2));

  readHandler.Data.clear();
  EXPECT_EQ(readerBP.ReadEx(readHandler), Reader::SUCCESS);
  EXPECT_TRUE(readHandler.Data.empty());
}

TEST_F(ReaderWriterTest, ScratchReadBufLen) {
  readerBP.Activate();

  for (int i = 1; i <= maxIntMsg; i++) {
    WriteInt(i);
  }

  EXPECT_EQ(readerBP.ReadEx(readHandler), Reader::SUCCESS);
  EXPECT_EQ(readHandler.Data.size(), maxIntMsg);
}

TEST_F(ReaderWriterTest, ScratchReadExpired) {
  readerBP.Activate();

  for (int i = 1; i <= maxIntMsg + 1; i++)
    WriteInt(i);

  EXPECT_EQ(readerBP.ReadEx(readHandler), Reader::EXPIRED_POSITION);
  EXPECT_EQ(readHandler.Data.size(), 0);
  EXPECT_EQ(readerInfo->stats.expiredReaders, 1);
}

TEST_F(ReaderWriterTest, SomeReadBufLen) {
  // some data
  WriteInt(0);

  readerBP.Activate();

  // wrap + padding
  // can hold 78 msgs. If freePos overlaps it may or may not be overwritten
  // as it is increased after writing. Assume it is.
  for (int i = 1; i <= maxIntMsg - 1; i++) {
    WriteInt(i);
  }

  EXPECT_EQ(readerBP.ReadEx(readHandler), Reader::SUCCESS);
  EXPECT_EQ(readHandler.Data.size(), maxIntMsg - 1);
}

TEST_F(ReaderWriterTest, WriteReadNoSpaceForHeader) {
  WriteBigData(noBPHandler);

  readerBP.Activate();
  // add padding, wrap
  WriteInt(10);

  EXPECT_EQ(readerBP.ReadEx(readHandler), Reader::SUCCESS);
  EXPECT_THAT(readHandler.Data, ElementsAre(10));
}

TEST_F(ReaderWriterTest, WriteBackpressure) {
  readerBP.Activate();

  WriteBigData(noBPHandler);

  EXPECT_CALL(mockBPHandler, Call(_, _)).Times(1).WillOnce(Return(false));

  // bp detected, overwrite
  WriteBigData(mockBPHandler.AsStdFunction());
  EXPECT_EQ(ringBuf->stats.backPressureCount, 1);

  // skip already expired
  WriteBigData(mockBPHandler.AsStdFunction());
  EXPECT_EQ(ringBuf->stats.backPressureCount, 1);
}

TEST_F(ReaderWriterTest, WriteNoActiveReaderNoBackpressure) {
  EXPECT_CALL(mockBPHandler, Call(_, _)).Times(0);

  WriteBigData(mockBPHandler.AsStdFunction());
  WriteBigData(mockBPHandler.AsStdFunction());

  EXPECT_EQ(ringBuf->stats.backPressureCount, 0);
}

TEST_F(ReaderWriterTest, ReadDenyCopyConfirm) {
  struct DenyCopyConfirmHandler {
    /*
     * Examples when false is returned
     *    - if copy was unsuccessful
     *    - data contains topic and we're not interested in it
     */
    bool Copy(const char *, size_t dataLen) {
      copyCount += dataLen;
      return false;
    }
    void Confirm() { confirmCount++; }

    int copyCount{0};
    int confirmCount{0};
  };

  readerBP.Activate();

  // allocate overwrite, no cnfirm
  DenyCopyConfirmHandler dcch;
  int bigDataSize = WriteBigData(noBPHandler);

  EXPECT_EQ(readerBP.ReadEx(dcch), Reader::SUCCESS);
  EXPECT_EQ(dcch.copyCount, bigDataSize);             // all data is processed
  EXPECT_EQ(readerBP.Pos(), ringBuf->freePos.load()); // all data is read
  EXPECT_EQ(dcch.confirmCount, 0);                    // no data is confirmed

  // the confirm and expire cases are covered by the read tests
}

TEST_F(ReaderWriterTest, TooManyBytesToWrite) {
  // With some data in the ring buf and large enough message,
  // padding is aplied and the total number of written bytes is
  // larger than the size of the buffer

  WriteInt(10);

  WriteBigData(noBPHandler);
}

TEST_F(ReaderWriterTest, WriteDieWriteDeathTest) {
  readerBP.Activate();
  WriteInt(20); // write some data

  // writer dies on backpressure while holding lock
  EXPECT_DEATH(
      {
        thread t([&]() {
          WriteBigData([](auto...) {
            // throw on backpressure
            throw 5;
            return false;
          });
        });
        t.join();
      },
      "");

  // next writer can acquire lock
  WriteInt(30);

  EXPECT_EQ(readerBP.ReadEx(readHandler), Reader::SUCCESS);
  EXPECT_THAT(readHandler.Data, ElementsAre(20, 30));
}

TEST_F(ReaderWriterTest, ReadInactiveDeathTest) {
  EXPECT_DEATH(readerBP.ReadEx(readHandler), "");
}

TEST_F(ReaderWriterTest, WriteBiggerMessageDeathTest) {
  vector<char> biggerMessage(writer.GetMaxMessageSize() + 1);
  EXPECT_DEATH(
      writer.WriteEx(biggerMessage.data(), biggerMessage.size(), noBPHandler),
      "");
}
