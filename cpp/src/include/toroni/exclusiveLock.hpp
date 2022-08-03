/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_EXCLUSIVELOCK_HPP
#define TORONI_EXCLUSIVELOCK_HPP

#include <mutex>

namespace toroni {

template <class T> using ExclusiveLock = std::lock_guard<T>;

} // namespace toroni

#endif // TORONI_EXCLUSIVELOCK_HPP
