/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_TRAITS_POSIX_ROBUSTPROCMUTEX_HPP
#define TORONI_TRAITS_POSIX_ROBUSTPROCMUTEX_HPP

#include <cassert>
#include <errno.h>
#include <pthread.h>

namespace toroni {
namespace traits {
/**
 * @brief  Robust process-shared mutex with auto recovery after last owned died.
 * @note
 * @retval None
 */
class RobustProcMutex {
public:
  /**
   * @brief Initialize a mutex.
   * @note The mutex must be initialized before used.
   * @param  &mtx:
   * @retval None
   */
  RobustProcMutex() noexcept {
    pthread_mutexattr_t ma;
    pthread_mutexattr_init(&ma);
    pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);

    if (pthread_mutex_init(&_mtx, &ma) != 0) {
      assert(false);
    }
  }

  /**
   * @brief  Locks a mutex or blocks if mutex is already locked.
   * @note
   * @param  &mtx:
   * @retval None
   */
  void Lock() noexcept {
    int r = pthread_mutex_lock(&_mtx);

    if (!SetConsistent(r) && r != 0) {
      assert(false);
    }
  }

  /**
   * @brief  Attempts to lock a mutex and returns immediately.
   * @note
   * @param  &mtx:
   * @retval true on success, false if already locked.
   */
  bool TryLock() noexcept {
    int r = pthread_mutex_trylock(&_mtx);

    if (r == EBUSY) {
      return false;
    } else if (r == 0 || SetConsistent(r)) {
      return true;
    } else {
      assert(false);
      return false;
    }
    return false;
  }

  /**
   * @brief  Unlocks a mutex
   * @note
   * @param  &mtx:
   * @retval None
   */
  void Unlock() noexcept {
    if (pthread_mutex_unlock(&_mtx) != 0) {
      assert(false);
    }
  }

  void lock() noexcept { Lock(); }
  void unlock() noexcept { Unlock(); }

private:
  pthread_mutex_t _mtx;

  /**
   * @brief  Marks the mutex as consistent if the last owner died before
   * unlocking
   * @note
   * @param  lockResult:
   * @retval True if owner has died. False otherwise.
   */
  bool SetConsistent(int lockResult) {
    if (lockResult == EOWNERDEAD) {
      if (pthread_mutex_consistent(&_mtx) != 0) {
        assert(false);
      }

      return true;
    } else {
      return false;
    }
  }
};
} // namespace traits
} // namespace toroni
#endif // TORONI_TRAITS_POSIX_ROBUSTPROCMUTEX_HPP
