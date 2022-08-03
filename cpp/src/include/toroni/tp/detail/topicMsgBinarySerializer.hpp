/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_TP_DETAIL_TOPICMSGBINARYSERIALIZER_HPP
#define TORONI_TP_DETAIL_TOPICMSGBINARYSERIALIZER_HPP

#include "toroni/tp/topicMsgBinary.hpp"

#include <cstring>

namespace toroni {
namespace tp {
/**
 * @brief  Binary serializer/deserializer for a topic message
 * @note   The layout is as follows:
 * 8b readerGen
 * 1b postToDescendants
 * Xb channelName (zero-terminated)
 * Yb data (to end of dataLen)
 * @retval None
 */
struct TopicMsgBinarySerializer {
  static uint32_t SizeOf(const std::string &channelName, uint32_t dataLen);
  static void Serialize(char *dst, uint64_t readerGen, bool postToDescendants,
                        const std::string &channelName, const void *data,
                        uint32_t dataLen);
};

/**
 * @brief  The size in bytes of the topic message including system data
 * @note
 * @param  &channelName: Topic
 * @param  dataLen: Data size in bytes
 * @retval Size in bytes
 */
inline uint32_t TopicMsgBinarySerializer::SizeOf(const std::string &channelName,
                                                 uint32_t dataLen) {
  return 8 + 1 + (channelName.size() + 1) + dataLen;
}

/**
 * @brief  Serialize to binary a topic message
 * @note
 * @param  *dst: Pre-allocated memory to serialize onto
 * @param  readerGen:
 * @param  postToDescendants:
 * @param  &channelName:
 * @param  *data:
 * @param  dataLen:
 * @retval None
 */
inline void TopicMsgBinarySerializer::Serialize(char *dst, uint64_t readerGen,
                                                bool postToDescendants,
                                                const std::string &channelName,
                                                const void *data,
                                                uint32_t dataLen) {
  auto readerGenPtr = reinterpret_cast<uint64_t *>(dst);
  *readerGenPtr = readerGen;
  readerGenPtr++;

  auto pdPtr = reinterpret_cast<char *>(readerGenPtr);
  *pdPtr = postToDescendants ? 1 : 0;
  pdPtr++;

  const size_t chLen = channelName.size() + 1;
  strncpy(pdPtr, channelName.c_str(), chLen);
  pdPtr += chLen;

  memcpy(pdPtr, data, dataLen);
}

} // namespace tp
} // namespace toroni

#endif // TORONI_TP_DETAIL_TOPICMSGBINARYSERIALIZER_HPP
