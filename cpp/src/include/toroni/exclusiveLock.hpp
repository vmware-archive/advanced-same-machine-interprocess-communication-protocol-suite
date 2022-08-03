#ifndef TORONI_EXCLUSIVELOCK_HPP
#define TORONI_EXCLUSIVELOCK_HPP

#include <mutex>

namespace toroni {

template <class T> using ExclusiveLock = std::lock_guard<T>;

} // namespace toroni

#endif // TORONI_EXCLUSIVELOCK_HPP
