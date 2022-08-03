/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_RMP_COPYCONFIRMHANDLER_HPP
#define TORONI_RMP_COPYCONFIRMHANDLER_HPP

#include <functional>
#include <vector>

namespace toroni {
namespace rmp {

/**
 * @brief  Copies data to storage and when confirmed invokes a callback.
 * @note
 * @retval None
 */
struct CopyConfirmHandler {
  using ReadCb = std::function<void(const char *data, size_t dataSize)>;

  explicit CopyConfirmHandler(const ReadCb &readCb) : _readCb(readCb) {}

  CopyConfirmHandler(size_t capacity, const ReadCb &readCb)
      : _data(capacity), _readCb(readCb) {}

  bool Copy(const char *data, size_t dataSize) {
    _data.assign(data, data + dataSize);
    // assign may reallocate
    return true;
  }

  void Confirm() const { _readCb(_data.data(), _data.size()); }

private:
  std::vector<char> _data;
  const ReadCb _readCb;
};

} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_COPYCONFIRMHANDLER_HPP
