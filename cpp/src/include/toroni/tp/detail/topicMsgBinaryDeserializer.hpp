#ifndef TORONI_TP_DETAIL_TOPICMSGBINARYDESERIALIZER_HPP
#define TORONI_TP_DETAIL_TOPICMSGBINARYDESERIALIZER_HPP

#include "topicMsgBinarySerializer.hpp"
#include "toroni/tp/topicMsgBinary.hpp"

#include <cassert>
#include <cstring>

namespace toroni {
namespace tp {
struct TopicMsgBinaryDeserializer {
  static bool DeserializeAndFilter(const char *&data, uint32_t &dataSize,
                                   uint64_t readerGen,
                                   const std::string &channelName,
                                   bool handleDescendants);

  static bool TopicMatches(const std::string &readerChannel,
                           bool handleDescendents, const char *writerChannel,
                           size_t writerChannelLength, bool postToDescendents);
};

/**
 * @brief  Deserialize a message if its topic matches
 * @note
 * @param  *&data: Pointer to the data within the message
 * @param  &dataSize: Size of the data within the message
 * @param  readerGen: Ignore messages younger than the reader
 * @param  &channelName: The expected channel name
 * @param  handleDescendants:
 * @retval True if topic matches. False otherwise.
 */
inline bool TopicMsgBinaryDeserializer::DeserializeAndFilter(
    const char *&data, uint32_t &dataSize, uint64_t readerGen,
    const std::string &channelName, bool handleDescendants) {
  assert(dataSize >= TopicMsgBinarySerializer::SizeOf("", 0));

  auto readerGenPtr = reinterpret_cast<const uint64_t *>(data);
  if (*readerGenPtr < readerGen) {
    return false;
  }
  readerGenPtr++;

  auto pdPtr = reinterpret_cast<const char *>(readerGenPtr);
  bool writerPd = *pdPtr;
  pdPtr++;

  const char *writerChName = pdPtr;
  size_t writerChNameLen = strlen(writerChName);

  if (TopicMatches(channelName, handleDescendants, writerChName,
                   writerChNameLen, writerPd)) {
    const char *odata = data;
    data = writerChName + writerChNameLen + 1;
    dataSize -= data - odata;

    return true;
  }

  return false;
}

/**
 * @brief  Match writer+postToDescendents and reader+handleDescendents topics
 * @note
 * @param  &readerChannel:
 * @param  handleDescendents:
 * @param  *writerChannel:
 * @param  writerChannelLength:
 * @param  postToDescendents:
 * @retval True if they match, false otherwise
 */
inline bool TopicMsgBinaryDeserializer::TopicMatches(
    const std::string &readerChannel, bool handleDescendents,
    const char *writerChannel, size_t writerChannelLength,
    bool postToDescendents) {
  size_t mlen = std::min(readerChannel.length(), writerChannelLength);
  size_t clen = 0;
  while (clen < mlen && readerChannel[clen] == writerChannel[clen]) {
    clen++;
  }

  if (clen == readerChannel.length() && clen == writerChannelLength) {
    return true;
  }

  if (postToDescendents && clen == writerChannelLength) {
    return true;
  }

  if (handleDescendents && clen == readerChannel.length()) {
    return true;
  }

  return false;
}

} // namespace tp
} // namespace toroni

#endif // TORONI_TP_DETAIL_TOPICMSGBINARYDESERIALIZER_HPP
