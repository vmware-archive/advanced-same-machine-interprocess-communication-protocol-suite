/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_TP_READERINFO_HPP
#define TORONI_TP_READERINFO_HPP

#include "toroni/rmp/readerInfo.hpp"

#include <atomic>

namespace toroni {
namespace tp {
using ReaderGen = uint64_t;

/**
 * @brief  Contains information about readers that can be used by writers to
 * * create backpressire
 * @note
 * @retval None
 */
struct ReaderInfo {
  /*
   * Header
   */
  std::atomic<ReaderGen> readerGen{0}; // Reader generation
  const bool initialized; // Whether this structure is completely initialized on
                          // a zero-initialized shared memory
  rmp::ReaderInfo rmpReaderInfo; // RMP Reader Info

  static uint32_t Size(uint16_t configMaxReaders);
  explicit ReaderInfo(uint16_t configMaxReaders);
};

/**
 * @brief  Returns the size in bytes.
 * @note
 * @param  configMaxReaders:
 * @retval The size in bytes.
 */
inline uint32_t ReaderInfo::Size(uint16_t configMaxReaders) {
  return sizeof(ReaderInfo) + rmp::ReaderInfo::Size(configMaxReaders) -
         sizeof(rmpReaderInfo) /*added twice*/;
}

/**
 * @brief Initialize a ReaderInfo with placement new
 * @note See Size
 * @param  configMaxReaders:
 */
inline ReaderInfo::ReaderInfo(uint16_t configMaxReaders)
    : rmpReaderInfo(configMaxReaders), initialized(true) {}
} // namespace tp
} // namespace toroni

#endif // TORONI_TP_READERINFO_HPP
