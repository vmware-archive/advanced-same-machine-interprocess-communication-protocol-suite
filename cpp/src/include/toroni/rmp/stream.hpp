/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_RMP_STREAM_HPP
#define TORONI_RMP_STREAM_HPP

#include <atomic>

namespace toroni {
namespace rmp {

/**
 * @brief  Stream is an infinite stream of bytes
 * @note
 * @retval None
 */
using Position = uint64_t;
using PositionAtomic = std::atomic<Position>;

using StatCounter = std::atomic<uint64_t>;

} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_STREAM_HPP
