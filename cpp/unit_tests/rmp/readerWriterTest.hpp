#ifndef READERWRITERTEST_HPP
#define READERWRITERTEST_HPP

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "toroni/rmp/reader.hpp"
#include "toroni/rmp/readerInfo.hpp"
#include "toroni/rmp/readerWithBackpressure.hpp"
#include "toroni/rmp/writer.hpp"

#include "byteRingBufferTest.hpp"
#include "readerInfoTest.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <new>
#include <vector>

namespace toroni {
namespace rmp {
namespace unit_tests {

struct ReadIntCopyConfirmHandler {
  bool Copy(const char *data, uint32_t dataSize) {
    _tmpData = *reinterpret_cast<const size_t *>(data);
    return true;
  }
  void Confirm() { Data.emplace_back(_tmpData); }

  std::vector<size_t> Data;

private:
  size_t _tmpData;
};

class ReaderWriterTest : virtual public ByteRingBufferTest,
                         virtual public ReaderInfoTest {
protected:
  ReaderWriterTest()
      : writer(ringBuf, readerInfo), reader(ringBuf),
        readerBP(ringBuf, readerInfo),
        noBPHandler([](auto...) { return false; }) {}

  Writer writer;
  Reader reader;
  ReaderWithBackpressure readerBP;

  ReadIntCopyConfirmHandler readHandler;

  Writer::BackPressureHandlerEx noBPHandler;
  ::testing::MockFunction<bool(toroni::rmp::Position, toroni::rmp::Position)>
      mockBPHandler;

  const size_t maxIntMsg{
      ringBufSizeBytes /
      (sizeof(toroni::rmp::detail::MsgHeader) + sizeof(size_t))};

  void WriteInt(size_t v) { writer.WriteEx(&v, sizeof(v), noBPHandler); }

  size_t
  WriteBigData(const toroni::rmp::Writer::BackPressureHandlerEx &bpHandler) {
    // one byte free left
    std::vector<char> bigData(ringBufSizeBytes -
                              sizeof(toroni::rmp::detail::MsgHeader) - 1);
    writer.WriteEx(bigData.data(), bigData.size(), bpHandler);
    return bigData.size();
  }
};

} // namespace unit_tests
} // namespace rmp
} // namespace toroni

#endif // READERWRITERTEST_HPP
