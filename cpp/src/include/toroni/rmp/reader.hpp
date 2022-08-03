/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_RMP_READER_HPP
#define TORONI_RMP_READER_HPP

#include "byteRingBuffer.hpp"
#include "detail/msgHeader.hpp"
#include "detail/util.hpp"
#include "readerInfo.hpp"
#include "stream.hpp"
#include "toroni/exception.hpp"

#include <cassert>
#include <functional>
#include <memory.h>

namespace toroni {
namespace rmp {

/**
 * @brief  Reliable Message Protocol (RMP) stream reader.
 * @note   Does not keep stream position or can create backpressure.
 * @retval None
 */
class Reader {
public:
  explicit Reader(const ByteRingBuffer *ringBuf);

  enum Result { SUCCESS, INVALID_POSITION, EXPIRED_POSITION };

  template <typename TCopyConfirmCb>
  Result ReadEx(PositionAtomic &pos, PositionAtomic &bpos,
                TCopyConfirmCb &copyConfirmCb);

private:
  const ByteRingBuffer *_ringBuf;
  const char *_ringBufData;
  const Position _indexMask;
};

inline Reader::Reader(const ByteRingBuffer *ringBuf)
    : _ringBuf(ringBuf), _ringBufData(&(*ringBuf)[0]),
      _indexMask(detail::IndexMask(ringBuf->configBufSizeBytes)) {
  if (!_ringBuf->initialized) {
    throw exception("Ring buffer not initialized");
  }
}

/**
 * @brief  Read starting from stream position pos until the stream end.
 * @note
 * @param  &pos: Stream position from which to start reading.
 * @param  &bpos: Update bpos with value of pos. Usually for purpose of
 * backpressure.
 * * @param  &copyConfirmCb: Invoke Copy for every message. If it returns
 * false, message is discarded. If it returns true, message processing
 * continues. Confirm is called if reader has not expired
 * @retval INVALID_POSITION if pos is after the stream end. EXPIRED_POSITION
 * * if the ring buffer has been overwritten at stream position pos. SUCCESS
 * * otherwise.
 */
template <typename TCopyConfirmCb>
Reader::Result Reader::ReadEx(PositionAtomic &pos, PositionAtomic &bpos,
                              TCopyConfirmCb &copyConfirmCb) {
  if (detail::GreaterThan(pos, _ringBuf->freePos)) {
    return INVALID_POSITION;
  }

  const Position bposMaxLag = _ringBuf->configBufSizeBytes >> 1;

  while (pos != _ringBuf->freePos) {
    Position bufIndex = detail::BufIndex(_indexMask, pos);

    if (_ringBuf->configBufSizeBytes - bufIndex < sizeof(detail::MsgHeader)) {
      // no place for header till end of buffer, skip these positions
      pos += _ringBuf->configBufSizeBytes - bufIndex;
      // bpos = pos.load(); let the main work
      continue;
    }

    // Early optimization expiration checks to avoid msg copying, unreliable
    if (detail::Expired(pos, _ringBuf->freePos, _ringBuf->configBufSizeBytes)) {
      // pos is wrapped
      return EXPIRED_POSITION;
    }

    detail::MsgHeader hdrCopy =
        *detail::Overlay<detail::MsgHeader>(&_ringBufData[bufIndex]);
    if (!hdrCopy.Valid() ||
        detail::Expired(pos, _ringBuf->freePos, _ringBuf->configBufSizeBytes)) {
      // invalid header, must have been overwritten
      return EXPIRED_POSITION;
    }

    assert(bufIndex + sizeof(detail::MsgHeader) + hdrCopy.length <=
           _ringBuf->configBufSizeBytes);

    // header is valid, copy message
    // if it is a normal one, not a PADDING message
    if (hdrCopy.type == detail::MsgHeader::MSG) {
      const char *dataPtr = &_ringBufData[bufIndex] + sizeof(detail::MsgHeader);

      if (copyConfirmCb.Copy(dataPtr, hdrCopy.length)) {
        // Reliable pessimistic expiration check.
        if (detail::Expired(pos, _ringBuf->freePos,
                            _ringBuf->configBufSizeBytes)) {
          // After msg was copied, pos is wrapped. This is pessimistic as
          // freePos is updated last in writing, so effectively it may not be
          // wrapped. Be pessimistic.
          return EXPIRED_POSITION;
        }

        copyConfirmCb.Confirm();
      }
    }

    pos += sizeof(detail::MsgHeader) + hdrCopy.length;

    if (pos >= bpos + bposMaxLag) {
      // stored in shared memory. update sparingly.
      bpos = pos.load();
    }
  }

  bpos = pos.load();

  return SUCCESS;
}

} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_READER_HPP
