/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_REF_HPP
#define TORONI_REF_HPP

#include <memory>

namespace toroni {
template <typename T> using Ref = std::shared_ptr<T>;
}

#endif // TORONI_REF_HPP
