#ifndef TORONI_RMP_READERINFO_HPP
#define TORONI_RMP_READERINFO_HPP

#include "stream.hpp"

#include <atomic>
#include <new>

namespace toroni {
namespace rmp {

/**
 * @brief  Dynamic allocation of a ReaderInfo slot
 * @note The slot lifecycle is Allocate-Activate-Deactivate-Free. Allocate/Free
 * * once in the process-lifetime.
 * @retval None
 */
class ReaderInfo {
public:
  /*
   * Header
   */
  struct Info {
    traits::RobustProcMutex lock; // Held for duration of proc reader
    PositionAtomic position{0};   // Position of proc reader
    std::atomic<uint8_t> isActive{false};
    // Whether used by at least one channel reader
  };

  struct {
    StatCounter expiredReaders{0};
  } stats;
  bool initialized{false};

  using ReaderId = int;
  static inline const ReaderId INVALID_READER_ID{-1};

  static uint32_t Size(uint16_t configMaxReaders);
  explicit ReaderInfo(uint16_t configMaxReaders);
  ReaderId Alloc();
  void Free(ReaderId readerId);
  void Activate(ReaderId readerId, Position pos);
  void Deactivate(ReaderId readerId);
  void GetActiveRange(uint16_t &min, uint16_t &max) const;
  const Info &Get(ReaderId readerId) const;
  Info &Get(ReaderId readerId);

private:
  uint16_t _configMaxReaders;
  std::atomic<uint32_t> _readersMinMax; // Range of active readers. [Min,Max)

  Info *InfoPtr() {
    return reinterpret_cast<Info *>(
        (reinterpret_cast<char *>(this) + sizeof(*this)));
  }
  const Info *InfoPtr() const {
    return reinterpret_cast<const Info *>(
        (reinterpret_cast<const char *>(this) + sizeof(*this)));
  }
  void UpdateActiveRange();
};

/**
 * @brief  Returns the size in bytes
 * @note
 * @param  configMaxReaders:
 * @retval Additional size in bytes
 */
inline uint32_t ReaderInfo::Size(uint16_t configMaxReaders) {
  return sizeof(ReaderInfo) + configMaxReaders * sizeof(Info);
}

/**
 * @brief Initialize a ByteRingBuffer with placement new
 * @note See Size
 * @param  configMaxReaders:
 */
inline ReaderInfo::ReaderInfo(uint16_t configMaxReaders)
    : _configMaxReaders(configMaxReaders), _readersMinMax(0) {

  // placement-new initialization of Info
  new (InfoPtr()) Info[_configMaxReaders];
  initialized = true;
}

/**
 * @brief Allocates a ReaderInfo slot and returns its id
 * @note Inter-proces and termination safe.
 * @retval INVALID_READER_ID if not free slot can be found.
 */
inline ReaderInfo::ReaderId ReaderInfo::Alloc() {
  Info *readersInfo = InfoPtr();

  for (int i = 0; i < _configMaxReaders; i++) {
    Info &readerInfo = readersInfo[i];

    if (readerInfo.lock.TryLock()) {
      // mark as inactive to clean up old value
      Deactivate(i);
      return i;
    }
  }

  return INVALID_READER_ID;
}

/**
 * @brief  Frees a ReaderInfo slot. It must be freed on the same thread where
 * * Alloc was called.
 * @note Inter-proces and termination safe.
 * @param  readerId:
 * @retval None
 */
inline void ReaderInfo::Free(ReaderInfo::ReaderId readerId) {
  if (readerId == INVALID_READER_ID) {
    return;
  }

  Info *readersInfo = InfoPtr();
  Deactivate(readerId);
  readersInfo[readerId].lock.Unlock();
}

/**
 * @brief  Activate an allocated reader info slot so its stream position is
 * taken into consideration for creating backpressure on writers.
 * @note Inter-proces and termination safe.
 * @param  readerId:
 * @param  pos: Set it to pos after activation
 * @retval INVALID_READER_ID if readerId is invalid
 */
inline void ReaderInfo::Activate(ReaderInfo::ReaderId readerId, Position pos) {
  if (readerId == INVALID_READER_ID) {
    return;
  }

  Info &readerInfo = InfoPtr()[readerId];
  readerInfo.position = pos;
  readerInfo.isActive = true;
  UpdateActiveRange();
}

/**
 * @brief  Deactivate an activated reader info slot so its stream position is n
 * longer taken into consideration for creating backpressure on writers
 * @note Inter-proces and termination safe.
 * @param  readerId:
 * @retval INVALID_READER_ID if readerId is invalid
 */
inline void ReaderInfo::Deactivate(ReaderInfo::ReaderId readerId) {
  if (readerId == INVALID_READER_ID) {
    return;
  }

  Info &readerInfo = InfoPtr()[readerId];
  readerInfo.isActive = false;
  UpdateActiveRange();
}

//
/**
 * @brief  Get the range of active slots.
 * @note   Interval is [min, max), some slots in it may be inactive.
 * @param  &min: Included
 * @param  &max: Excluded
 * @retval None
 */
inline void ReaderInfo::GetActiveRange(uint16_t &min, uint16_t &max) const {
  max = _readersMinMax;
  min = _readersMinMax >> 16;
}

/**
 * @brief  Get reader info slot
 * @note
 * @param  readerId:
 * @retval Reader info slot
 */
inline const ReaderInfo::Info &
ReaderInfo::Get(ReaderInfo::ReaderId readerId) const {
  assert(readerId >= 0 && readerId < _configMaxReaders);

  const Info *readerInfo = InfoPtr();

  return readerInfo[readerId];
}

inline ReaderInfo::Info &ReaderInfo::Get(ReaderInfo::ReaderId readerId) {
  assert(readerId >= 0 && readerId < _configMaxReaders);

  Info *readerInfo = InfoPtr();

  return readerInfo[readerId];
}

/**
 * @brief  Update the range of active slots
 * @note
 * @retval None
 */
inline void ReaderInfo::UpdateActiveRange() {
  Info *readerInfo = InfoPtr();

  uint32_t packed = 0;
  uint32_t old = 0;
  do {
    old = _readersMinMax;

    uint16_t min = _configMaxReaders, max = 0;

    for (int i = 0; i < _configMaxReaders; i++) {
      if (readerInfo[i].isActive) {
        if (i < min) {
          min = i;
        }
        if (i > max) {
          max = i;
        }
      }
    }

    packed = min << 16 | (max + 1);

  } while (!_readersMinMax.compare_exchange_weak(old, packed));
}

} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_READERINFO_HPP
