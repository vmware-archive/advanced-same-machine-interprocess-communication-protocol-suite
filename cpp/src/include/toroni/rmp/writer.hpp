/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_RMP_WRITER_HPP
#define TORONI_RMP_WRITER_HPP

#include "byteRingBuffer.hpp"
#include "detail/msgHeader.hpp"
#include "detail/positionOpt.hpp"
#include "detail/util.hpp"
#include "readerInfo.hpp"
#include "stream.hpp"
#include "toroni/exception.hpp"
#include "toroni/exclusiveLock.hpp"

#include <cassert>
#include <functional>
#include <memory.h>

namespace toroni {
namespace rmp {

/**
 * @brief  Reliable Message Protocol (RMP)
 * @note A stream reader that can detect backpressure by readers
 * @retval None
 */
class Writer {
public:
  using BackPressureHandlerEx =
      std::function<bool(Position bpPos, Position freePos)>;

  Writer(ByteRingBuffer *ringBuf, const ReaderInfo *readerInfo);
  void WriteEx(const void *data, uint32_t dataSize,
               const BackPressureHandlerEx &bpHandler);
  uint64_t GetMaxMessageSize() const;

private:
  detail::PositionOpt Write(const void *data, uint32_t dataSize, bool readerBP,
                            const detail::PositionOpt &skipConsPos);
  detail::PositionOpt
  DetectReaderBackpressure(Position n, const detail::PositionOpt &skipConsPos);

  ByteRingBuffer *_ringBuf;
  char *_ringBufData;
  const ReaderInfo *_readerInfo;
  const Position _indexMask;
};

inline Writer::Writer(ByteRingBuffer *ringBuf, const ReaderInfo *readerInfo)
    : _ringBuf(ringBuf), _ringBufData(&(*ringBuf)[0]), _readerInfo(readerInfo),
      _indexMask(detail::IndexMask(ringBuf->configBufSizeBytes)) {
  if (!_ringBuf->initialized) {
    throw exception("Ring buffer not initialized");
  }
}

/**
 * @brief  Write a message to the stream.
 * @note
 * @param  *data:
 * @param  dataSize:
 * @param  &bpHandler: Invoked if backpressure is detected. If it returns false,
 * write the
 * message anyhow. If it returns true, continue trying taking backpressure
 * into account.
 * @retval None
 */
inline void Writer::WriteEx(const void *data, uint32_t dataSize,
                            const BackPressureHandlerEx &bpHandler) {
  ExclusiveLock<traits::RobustProcMutex> lk(_ringBuf->writerMtx);

  // Run the loop until either the write succeeds without detecting
  // backpressure or until the BP handler returns false, meaning write the
  // message without thinking about backpressure.
  detail::PositionOpt readBp;
  while ((readBp = Write(data, dataSize, true, detail::EMPTY_POSITION))) {
    _ringBuf->stats.backPressureCount++;
    bool continueRunning = bpHandler(0, _ringBuf->freePos.load());
    if (!continueRunning) {
      Write(data, dataSize, false, detail::EMPTY_POSITION);
      break;
    }
  }
}

/**
 * @brief  Return the maximal useful message size that can be placed in the ring
 * buffer taking into account system data that is also placed.
 * @note
 * @retval Size in bytes
 */
inline uint64_t Writer::GetMaxMessageSize() const {
  return _ringBuf->configBufSizeBytes - sizeof(detail::MsgHeader);
}

/**
 * @brief  Write to stream
 * @note
 * @param  *data:
 * @param  dataSize:
 * @param  readerBP: If true consider backpressure by readers.
 * @param  &skipConsPos: Skip reader positions after skipConsPos when checking
 * for backpressure
 * @retval Return EMPTY_POSITION of no backpressure was detected. Otherwise,
 * return a reader position that causes backpressure.
 */
inline detail::PositionOpt
Writer::Write(const void *data, uint32_t dataSize, bool readerBP,
              const detail::PositionOpt &skipConsPos) {

  assert(dataSize <= GetMaxMessageSize());

  // estimate number of bytes to write
  Position bufIndex = detail::BufIndex(_indexMask, _ringBuf->freePos);
  Position lengthToBufEnd = _ringBuf->configBufSizeBytes - bufIndex;

  Position bytesToWrite = sizeof(detail::MsgHeader) + dataSize;
  bool addPadding = false;
  bool addBlank = false;

  if (lengthToBufEnd < sizeof(detail::MsgHeader)) {
    // no place for header to end of buf, skip these positions
    addBlank = true;
    bytesToWrite += lengthToBufEnd;
  } else if (lengthToBufEnd < dataSize + sizeof(detail::MsgHeader)) {
    // no place for msg to end of buf, add padding msg
    addPadding = true;
    bytesToWrite += lengthToBufEnd;
  }

  // check for bp
  if (readerBP) {
    detail::PositionOpt readerBpPos =
        DetectReaderBackpressure(bytesToWrite, skipConsPos);
    if (readerBpPos) {
      return readerBpPos;
    }
  }

  // write blank or padding message
  if (addPadding) {
    // padding msg
    detail::MsgHeader *header =
        detail::Overlay<detail::MsgHeader>(_ringBufData + bufIndex);

    header->type = detail::MsgHeader::PADDING;
    header->length = lengthToBufEnd - sizeof(detail::MsgHeader);

    _ringBuf->freePos += lengthToBufEnd;
    bufIndex = 0;
  } else if (addBlank) {
    // blank msg
    _ringBuf->freePos += lengthToBufEnd;
    bufIndex = 0;
  }

  // write the message
  detail::MsgHeader *rbHeader =
      detail::Overlay<detail::MsgHeader>(_ringBufData + bufIndex);
  rbHeader->type = detail::MsgHeader::MSG;
  rbHeader->length = dataSize;
  char *rbData = reinterpret_cast<char *>(rbHeader) + sizeof(detail::MsgHeader);
  memcpy(rbData, data, dataSize);

  _ringBuf->freePos += sizeof(detail::MsgHeader) + dataSize;

  return detail::EMPTY_POSITION;
}

/**
 * @brief  Detect if writing n bytes and ignoring reader positions <=
 * skipConsPos will expire a reader.
 * @note
 * @param  n:
 * @param  &skipConsPos:
 * @retval non-empty position that will be expired or empty position otherwise.
 */
inline detail::PositionOpt
Writer::DetectReaderBackpressure(Position n,
                                 const detail::PositionOpt &skipConsPos) {
  // _freePos under lock

  const bool skipConsPosEmpty = !skipConsPos;
  const Position skipConsPosValue = skipConsPosEmpty ? 0 : skipConsPos.value();

  uint16_t min = 0, max = 0;
  _readerInfo->GetActiveRange(min, max);
  // Best-effort loop only active slots
  for (int i = min; i < max; i++) {
    const auto &readerInfo = _readerInfo->Get(i);

    if (!readerInfo.isActive) {
      continue;
    }

    Position readerPos = readerInfo.position;

    if (detail::Expired(readerPos, _ringBuf->freePos,
                        _ringBuf->configBufSizeBytes)) {
      // reader already has expired. don't consider it for bp
      continue;
    }

    if (skipConsPosEmpty || detail::GreaterThan(readerPos, skipConsPosValue)) {
      if (detail::Expired(readerPos, _ringBuf->freePos + n,
                          _ringBuf->configBufSizeBytes)) {
        return readerPos;
      }
    }
  }

  return detail::EMPTY_POSITION;
}
} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_WRITER_HPP
