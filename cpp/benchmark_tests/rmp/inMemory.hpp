#include "toroni/rmp/reader.hpp"
#include "toroni/rmp/readerWithBackpressure.hpp"
#include "toroni/rmp/writer.hpp"

#include <vector>

namespace toroni {
namespace rmp {
namespace bench {

using namespace std;

class InMemoryRingBuf {
public:
  InMemoryRingBuf(int64_t bufSizeBytes)
      : ringBufSizeBytes(bufSizeBytes),
        ringBufMem(ByteRingBuffer::Size(bufSizeBytes)),
        _ringBuf(new (ringBufMem.data()) ByteRingBuffer(bufSizeBytes)) {}

  ByteRingBuffer *Get() const { return _ringBuf; }

private:
  size_t ringBufSizeBytes;
  std::vector<char> ringBufMem;
  ByteRingBuffer *const _ringBuf;
};

class InMemoryReaderInfo {
public:
  InMemoryReaderInfo(int readerInfoSlots)
      : readerInfoMem(ReaderInfo::Size(readerInfoSlots)),
        readerInfo(new (readerInfoMem.data()) ReaderInfo(readerInfoSlots)) {}

  ReaderInfo *Get() const { return readerInfo; }

private:
  std::vector<char> readerInfoMem;
  ReaderInfo *const readerInfo;
};

} // namespace bench
} // namespace rmp
} // namespace toroni