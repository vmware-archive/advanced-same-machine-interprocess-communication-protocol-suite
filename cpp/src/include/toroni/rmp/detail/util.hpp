/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_RMP_DETAIL_UTIL_HPP
#define TORONI_RMP_DETAIL_UTIL_HPP

namespace toroni {
namespace rmp {
namespace detail {
template <typename T> static T *Overlay(char *buf) {
  return reinterpret_cast<T *>(buf);
}
template <typename T> static const T *Overlay(const char *buf) {
  return reinterpret_cast<const T *>(buf);
}

static Position BufIndex(Position indexMask, Position pos) {
  return pos & indexMask;
}
static bool GreaterThan(Position a, Position b) { return static_cast<int64_t>(b - a) < 0; }
static bool GreaterOrEqualThan(Position a, Position b) {
  return static_cast<int64_t>(b - a) <= 0;
}
static bool Expired(Position pos, Position freePos, Position bufSize) {
  return GreaterOrEqualThan(freePos, pos + bufSize);
}
static bool IsZeroOrPowerOfTwo(Position x) { return !(x & (x - 1)); }

static Position IndexMask(Position bufSizePowOf2) {
  assert(IsZeroOrPowerOfTwo(bufSizePowOf2));
  return bufSizePowOf2 - 1;
}
} // namespace detail
} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_DETAIL_UTIL_HPP
