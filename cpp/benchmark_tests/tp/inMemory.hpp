#include "toroni/tp/asyncWriter.hpp"
#include "toroni/tp/reader.hpp"

#include <vector>

namespace toroni {
namespace tp {
namespace bench {

using namespace std;

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
} // namespace tp
} // namespace toroni