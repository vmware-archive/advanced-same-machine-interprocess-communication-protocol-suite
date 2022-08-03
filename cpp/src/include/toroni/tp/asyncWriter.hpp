#ifndef TORONI_TP_ASYNCWRITER_HPP
#define TORONI_TP_ASYNCWRITER_HPP

#include "detail/topicMsgBinarySerializer.hpp"
#include "readerInfo.hpp"
#include "toroni/exception.hpp"
#include "toroni/ref.hpp"
#include "toroni/rmp/writer.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace toroni {
namespace tp {

/**
 * @brief Topic Protocol (TP) async message writer
 * @note
 * @retval None
 */
class AsyncWriter : public std::enable_shared_from_this<AsyncWriter> {
public:
  using EnueueMsgFn = std::function<bool(const TopicMsgBinaryPtr &)>;
  using DrainMsgFn = std::function<std::vector<TopicMsgBinaryPtr>()>;

  using WorkFn = std::function<void()>;
  using EnqueueWorkFn = std::function<void(const WorkFn &)>;
  using NotifyAllReadersFn = std::function<void()>;

  static Ref<AsyncWriter>
  Create(rmp::ByteRingBuffer *ringBuf, const tp::ReaderInfo *readerInfo,
         const EnueueMsgFn &enqueueMsgFn, const DrainMsgFn &drainMsgFn,
         const EnqueueWorkFn &enqueueWorkFn,
         const rmp::Writer::BackPressureHandlerEx &backPressureFn,
         const NotifyAllReadersFn &notifyAllReadersFn);

  TopicMsgBinaryPtr CreateMessage(const char *channelName, const void *msg,
                                  size_t msgLen, bool postToDescendants);

  void Post(const TopicMsgBinaryPtr &rbMsg);

private:
  rmp::ByteRingBuffer *_ringBuf;
  const ReaderInfo *_readerInfo;
  rmp::Writer _rbWriter;
  EnueueMsgFn _enqueueMsgFn;
  DrainMsgFn _drainMsgFn;
  std::function<void(const std::function<void()>)> _enqueueWorkFn;
  rmp::Writer::BackPressureHandlerEx _backpressureFn;
  std::function<void()> _notifyAllReaders;

  AsyncWriter(rmp::ByteRingBuffer *ringBuf, const tp::ReaderInfo *readerInfo,
              const EnueueMsgFn &enqueueMsgFn, const DrainMsgFn &drainMsgFn,
              const EnqueueWorkFn &enqueueWorkFn,
              const rmp::Writer::BackPressureHandlerEx &backPressureFn,
              const NotifyAllReadersFn &notifyAllReadersFn)
      : _ringBuf(ringBuf), _readerInfo(readerInfo),
        _rbWriter(_ringBuf, &_readerInfo->rmpReaderInfo),
        _enqueueMsgFn(enqueueMsgFn), _drainMsgFn(drainMsgFn),
        _enqueueWorkFn(enqueueWorkFn), _backpressureFn(backPressureFn),
        _notifyAllReaders(notifyAllReadersFn) {}
  void ProcWriter();
};

/**
 * @brief  Create an async topic message writer
 * @note
 * @param  *ringBuf:
 * @param  *readerInfo:
 * @param  &enqueueMsgFn: Thread-safe function to enqueue message.
 * @param  &drainMsgFn: Thread-safe function to dequeue all message.
 * @param  &enqueueWorkFn: Thread-safe function to enqueue work
 * @param  &backPressureFn: Invoked when backpressure is detected
 * @param  &notifyAllReadersFn: Invoked to send notification to all readers to
 * start reading
 * * @param  notifyDenominator: Notification is send after using
 * 1/notifyDenominator-th of the ring buffer
 * @retval Async topic message writer
 */
inline Ref<AsyncWriter> AsyncWriter::Create(
    rmp::ByteRingBuffer *ringBuf, const tp::ReaderInfo *readerInfo,
    const EnueueMsgFn &enqueueMsgFn, const DrainMsgFn &drainMsgFn,
    const EnqueueWorkFn &enqueueWorkFn,
    const rmp::Writer::BackPressureHandlerEx &backPressureFn,
    const NotifyAllReadersFn &notifyAllReadersFn) {
  return std::shared_ptr<AsyncWriter>(
      new AsyncWriter(ringBuf, readerInfo, enqueueMsgFn, drainMsgFn,
                      enqueueWorkFn, backPressureFn, notifyAllReadersFn));
}

/**
 * @brief  Create a topic message
 * @note
 * @param  *channelName:
 * @param  *msg: Pointer to message data
 * @param  msgLen: Size of message data
 * @param  postToDescendants: Whether to post to descendants. E.g. if set to
 * true and posting to channel /a, then readers of /a/'*' will also get the
 * message.
 * @retval Topic message. Throws if message data is too big.
 */
inline TopicMsgBinaryPtr AsyncWriter::CreateMessage(const char *channelName,
                                                    const void *msg,
                                                    size_t msgLen,
                                                    bool postToDescendants) {
  assert(msgLen != 0);

  const size_t topicMsgLen =
      TopicMsgBinarySerializer::SizeOf(channelName, msgLen);

  if (topicMsgLen > _rbWriter.GetMaxMessageSize()) {
    throw exception("Message size exceeds ringbuffer size");
  }

  TopicMsgBinaryPtr rbMsg = std::make_shared<TopicMsgBinary>(topicMsgLen);
  TopicMsgBinarySerializer::Serialize(&(*rbMsg)[0], _readerInfo->readerGen,
                                      postToDescendants, channelName, msg,
                                      msgLen);
  return rbMsg;
}

/**
 * @brief  Posts a message and returns immediateliy.
 * * @note   Invokes enqueueWorkFn with ProcWriter if none is running. The
 * actual writing to the ring buffer must be on another thread.
 * @param  &rbMsg:
 * @retval None
 */
inline void AsyncWriter::Post(const TopicMsgBinaryPtr &rbMsg) {
  assert(rbMsg->size() < _rbWriter.GetMaxMessageSize());

  if (_enqueueMsgFn(rbMsg)) {
    _enqueueWorkFn([ref = shared_from_this()]() { ref->ProcWriter(); });
  }
}

/**
 * @brief  The actual writing to the ring buffer.
 * @note   At most one ProcWriter is running. Exits when there are no messages
 * to write.
 * @retval None
 */
inline void AsyncWriter::ProcWriter() {
  const rmp::Writer::BackPressureHandlerEx bpWrapper = [&](const auto... p) {
    // Notify readers on backpressure
    _ringBuf->stats.notificationCount++;
    _notifyAllReaders();
    // Invoke backpressure callback
    return _backpressureFn(p...);
  };

  while (true) {
    std::vector<TopicMsgBinaryPtr> queueCopy{_drainMsgFn()};

    if (queueCopy.empty()) {
      // Notify when this loop ends, otherwise on backpressure
      _ringBuf->stats.notificationCount++;
      _notifyAllReaders();
      break;
    }

    for (const auto &msg : queueCopy) {
      _rbWriter.WriteEx(&(*msg)[0], msg->size(), bpWrapper);
    }
  }
}

} // namespace tp
} // namespace toroni

#endif // TORONI_TP_ASYNCWRITER_HPP
