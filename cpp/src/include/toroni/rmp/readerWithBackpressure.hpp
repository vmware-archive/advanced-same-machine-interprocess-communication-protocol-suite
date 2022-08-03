#ifndef TORONI_RMP_READERWITHBACKPRESSURE_HPP
#define TORONI_RMP_READERWITHBACKPRESSURE_HPP

#include "reader.hpp"
#include "toroni/exception.hpp"

#include <cassert>

namespace toroni {
namespace rmp {

/**
 * @brief  Reliable Message Protocol (RMP) stream reader with backpressure.
 *
 * @note A stream reader that maintains in-process its stream position and
 * updates a reader info with it for the purpose of backpressure.
 * @retval None
 */
class ReaderWithBackpressure {
public:
  ReaderWithBackpressure(const ByteRingBuffer *ringBuf, ReaderInfo *readerInfo);
  ~ReaderWithBackpressure();
  void Activate();
  void Deactivate();
  bool IsActive() const;
  Position Pos() const;
  template <typename TCopyConfirmCb>
  Reader::Result ReadEx(TCopyConfirmCb &copyConfirmCb);

private:
  const ByteRingBuffer *_ringBuf;
  Reader _reader;
  ReaderInfo *_readerInfo;
  ReaderInfo::Info *_info;
  const Position _indexMask;
  ReaderInfo::ReaderId _procReaderId;
  PositionAtomic _readerPos; // in-process stream position of this reader.
};

/**
 * @brief  Initialize ReaderWithBackpressure
 * @note   Allocate a reader info slot
 * @param  *ringBuf:
 * @param  *readerInfo:
 * @retval Throws if no free slot is found
 */
inline ReaderWithBackpressure::ReaderWithBackpressure(
    const ByteRingBuffer *ringBuf, ReaderInfo *readerInfo)
    : _ringBuf(ringBuf), _reader(ringBuf), _readerInfo(readerInfo),
      _indexMask(detail::IndexMask(ringBuf->configBufSizeBytes)) {

  if (!_readerInfo->initialized) {
    throw exception("Rmp reader info not initialized");
  }

  _procReaderId = _readerInfo->Alloc();
  if (_procReaderId == ReaderInfo::INVALID_READER_ID) {
    throw exception("Unable to allocate proc reader info");
  }
  _info = &_readerInfo->Get(_procReaderId);
}

inline ReaderWithBackpressure::~ReaderWithBackpressure() {
  // No handling of running proc reader & writer threads. Assume proc
  // lifeftime.
  //    - for simplicity. They can hold a shared ptr to this object
  //    - depends on shutdown strategy of the application

  _readerInfo->Free(_procReaderId);
}

/**
 * @brief  Activate a reader info slot. Initialize in-process reader position to
 * * stream end
 * @note
 * @retval None
 */
inline void ReaderWithBackpressure::Activate() {
  // init local reader pos with stream position
  _readerPos = _ringBuf->freePos.load();
  _readerInfo->Activate(_procReaderId, _readerPos);
}

/**
 * @brief  Deactivate the allocated reader info slot
 * @note
 * @retval None
 */
inline void ReaderWithBackpressure::Deactivate() {
  _readerInfo->Deactivate(_procReaderId);
}

/**
 * @brief  Check if the reader info slot is active
 * @note
 * @retval True if active, false otherwise
 */
inline bool ReaderWithBackpressure::IsActive() const {
  const ReaderInfo::Info *info = &_readerInfo->Get(_procReaderId);

  return info->isActive;
}

/**
 * @brief  The in-process reader position
 * @note
 * @retval Reader stream position
 */
inline Position ReaderWithBackpressure::Pos() const {
  return _readerPos.load();
}

/**
 * @brief  Read from the reader stream position until the stream end
 * @note
 * @param  &copyConfirmCb: Invoke Copy for every message. If it returns
 * false, message is discarded. If it returns true, message processing
 * continues. Confirm is called if reader has not expired
 * @retval EXPIRED_POSITION if the reader stream position is before the stream
 * end and the ring buffer has been overwritten. SUCCESS otherwise.
 */
template <typename TCopyConfirmCb>
inline Reader::Result
ReaderWithBackpressure::ReadEx(TCopyConfirmCb &copyConfirmCb) {
  assert(IsActive());

  Reader::Result result =
      _reader.ReadEx(_readerPos, _info->position, copyConfirmCb);

  assert(result != Reader::Result::INVALID_POSITION);

  if (result == Reader::Result::EXPIRED_POSITION) {
    _readerInfo->stats.expiredReaders++;
  }

  return result;
}

} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_READERWITHBACKPRESSURE_HPP
