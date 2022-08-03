/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_TP_READER_HPP
#define TORONI_TP_READER_HPP

#include "channelReader.hpp"
#include "detail/topicMsgBinaryDeserializer.hpp"
#include "toroni/exception.hpp"
#include "toroni/rmp/byteRingBuffer.hpp"
#include "toroni/rmp/copyConfirmHandler.hpp"
#include "toroni/rmp/readerWithBackpressure.hpp"
#include "toroni/rmp/stream.hpp"
#include "toroni/tp/readerInfo.hpp"

#include <memory>
#include <string>
#include <vector>

namespace toroni {
namespace tp {

using EnqueueSerialFn = std::function<void(const std::function<void()> &)>;

enum ChannelReaderEventType {
  FIRST_CHANNEL_READER_CREATED,
  LAST_CHANNEL_READER_CLOSED,
  ALL_CHANNEL_READERS_EXPIRED
};
using ChannelReaderEventCb = std::function<void(ChannelReaderEventType)>;

/**
 * @brief  Topic Protocol (TP) message reader.
 * @note
 * @retval None
 */
class Reader : public std::enable_shared_from_this<Reader> {
public:
  static Ref<Reader> Create(const rmp::ByteRingBuffer *ringBuf,
                            ReaderInfo *readerInfo,
                            const EnqueueSerialFn &serialReadFn,
                            const EnqueueSerialFn &rmpReadFn,
                            const ChannelReaderEventCb &eventCb);

  Ref<ChannelReader> CreateChannelReader(const std::string &name,
                                         const ChannelReader::Handler &fun,
                                         bool handleDescendents);
  void CloseChannelReader(const ChannelReader *channelReader);
  void Run();

private:
  EnqueueSerialFn _enqueueSerialReader;
  EnqueueSerialFn _enqueueRmpRead;
  ReaderInfo *_readerInfo;
  rmp::ReaderWithBackpressure _rmpReaderBP;
  // Modified only by _enqueueRmpRead
  std::vector<Ref<ChannelReader>> _channelReaders;
  // modified only by _enqueueSerialReader
  ChannelReaderEventCb _channelReaderEventCb;

  Reader(const rmp::ByteRingBuffer *ringBuf, ReaderInfo *readerInfo,
         const EnqueueSerialFn &serialReadFn, const EnqueueSerialFn &rmpReadFn,
         const ChannelReaderEventCb &eventCb);

  // Scheduled only on _enqueueRmpRead
  void ReadRmp(const std::vector<Ref<ChannelReader>> &channelReaders);
  void HandleExpiredProcReader(rmp::Position pos);
  // Scheduled only on _enqueueSerialReader
  void AddChannelReader(const Ref<ChannelReader> &channelReader);
  void RemoveChannelReader(const ChannelReader *channelReader);
  std::vector<Ref<ChannelReader>> GetChannelReaders() const;
};

inline Ref<Reader> Reader::Create(const rmp::ByteRingBuffer *ringBuf,
                                  ReaderInfo *readerInfo,
                                  const EnqueueSerialFn &serialReadFn,
                                  const EnqueueSerialFn &rmpReadFn,
                                  const ChannelReaderEventCb &eventCb) {
  return Ref<Reader>(
      new Reader(ringBuf, readerInfo, serialReadFn, rmpReadFn, eventCb));
}

inline Reader::Reader(const rmp::ByteRingBuffer *ringBuf,
                      ReaderInfo *readerInfo,
                      const EnqueueSerialFn &serialReadFn,
                      const EnqueueSerialFn &rmpReadFn,
                      const ChannelReaderEventCb &eventCb)
    : _readerInfo(readerInfo),
      _rmpReaderBP(ringBuf, &readerInfo->rmpReaderInfo),
      _enqueueSerialReader(serialReadFn), _enqueueRmpRead(rmpReadFn),
      _channelReaderEventCb(eventCb) {
  if (!readerInfo->initialized) {
    throw exception("TP reader info not initialized");
  }
}

/**
 * @brief  Create a channel reader for a topic.
 * @note
 * @param  &name: Topic
 * @param  &fun: Invoked with messages for this topic
 * @param  handleDescendents:
 * @retval The channel reader
 */
inline Ref<ChannelReader>
Reader::CreateChannelReader(const std::string &name,
                            const ChannelReader::Handler &fun,
                            bool handleDescendents) {
  Ref<ChannelReader> result = std::make_shared<ChannelReader>(
      name, fun, handleDescendents, _readerInfo->readerGen);

  _enqueueSerialReader(
      [=, self = shared_from_this()]() { self->AddChannelReader(result); });

  return result;
}

/**
 * @brief  Close an existing channel reader.
 * @note   A no-op if the reader does not exist.
 * @param  *channelReader: The channel reader to remove
 * @retval None
 */
inline void Reader::CloseChannelReader(const ChannelReader *channelReader) {
  _enqueueSerialReader([=, self = shared_from_this()]() {
    self->RemoveChannelReader(channelReader);
  });
}

/**
 * @brief  Run starts a RMP reader with the current set of channel readers.
 * @note   Thread safe. Typicall called only when notification by writers is
 * received.
 * @retval None
 */
inline void Reader::Run() {
  _enqueueSerialReader([self = shared_from_this()]() {
    auto channelReadersCopy = self->GetChannelReaders();

    if (!channelReadersCopy.empty()) {
      self->_enqueueRmpRead([=]() { self->ReadRmp(channelReadersCopy); });
    }
  });
}

/**
 * @brief  Reads from RMP and sends messages to the channel readers for
 * filtering.
 * @note   Thread unsafe. At most one invocation may run at a time.  Scheduled
 * for execution on the ReadRmp serial work queue. Exits when there are no RMP
 * messages to read or the RMP reader expires.
 * @param  &channelReaders: A copy of channel readers that existed at the time
 * @retval None
 */
inline void
Reader::ReadRmp(const std::vector<Ref<ChannelReader>> &channelReaders) {
  assert(!channelReaders.empty());

  if (!_rmpReaderBP.IsActive()) {
    // called before first channel reader was created, or after all are
    // closed. In case 2 a notification being processed slowly by this method
    // may lead to missed notifications as it will bail out here.
    return;
  }

  rmp::CopyConfirmHandler cch([&](const char *data, uint32_t dataSize) {
    for (const auto &cr : channelReaders) {
      // copy, modified
      const char *dataCopy = data;
      uint32_t dataSizeCopy = dataSize;

      if (TopicMsgBinaryDeserializer::DeserializeAndFilter(
              dataCopy, dataSizeCopy, cr->_readerGen, cr->_name,
              cr->_handleDescendents)) {
        cr->_handler(dataCopy, dataSizeCopy);
      }
    }
  });

  auto res = _rmpReaderBP.ReadEx(cch);

  if (res == rmp::Reader::EXPIRED_POSITION) {
    HandleExpiredProcReader(_rmpReaderBP.Pos());
  }
}

inline void Reader::HandleExpiredProcReader(rmp::Position  /*pos*/) {
  _channelReaderEventCb(ALL_CHANNEL_READERS_EXPIRED);
}

/**
 * @brief  Add a channel reader to the list.
 * @note   Thread-unsafe. Scheduled on the reader serial work queue. Activate
 * the rmp reader on the first channel reader.
 * @param  &channelReader:
 * @retval None
 */
inline void Reader::AddChannelReader(const Ref<ChannelReader> &channelReader) {
  _channelReaders.push_back(channelReader);

  if (_channelReaders.size() == 1) {
    _rmpReaderBP.Activate();

    _channelReaderEventCb(FIRST_CHANNEL_READER_CREATED);
  }
}

/**
 * @brief  Remove a channel reader from the list.
 * @note   Thread-unsafe. Scheduled on the reader serial work queue. Dctivate
 * the rmp reader after the last channel reader
 * @param  &channelReader:
 * @retval None
 */
inline void Reader::RemoveChannelReader(const ChannelReader *channelReader) {
  bool lastReader = false;
  for (auto it = _channelReaders.begin(); it != _channelReaders.end(); it++) {
    if (it->get() == channelReader) {
      _channelReaders.erase(it);
      lastReader = _channelReaders.empty();
      break;
    }
  }
  if (lastReader) {
    _rmpReaderBP.Deactivate();
    _channelReaderEventCb(LAST_CHANNEL_READER_CLOSED);
  }
}

/**
 * @brief  Get the list of channel readers.
 * @note    Thread-unsafe. Scheduled on the reader serial work queue.
 * @retval The list of channel readers.
 */
inline std::vector<Ref<ChannelReader>> Reader::GetChannelReaders() const {
  return _channelReaders;
}

} // namespace tp
} // namespace toroni

#endif // TORONI_TP_READER_HPP
