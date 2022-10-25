/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.traits;

import com.sun.jna.Pointer;
import com.sun.jna.Memory;
import com.sun.jna.Library;
import com.sun.jna.Native;

import static com.sun.jna.platform.linux.ErrNo.EBUSY;
import static com.sun.jna.platform.linux.ErrNo.EOWNERDEAD;

/**
 * Robust process-shared mutex with auto recovery if last owner has died.
 */
public class PthreadRobustMutex implements RobustMutex {

  private static Pthread PTHREAD = Pthread.INSTANCE;

  private Pointer _mtx;

  @Override
  public RobustMutex load(Pointer mtx) {
    PthreadRobustMutex result = new PthreadRobustMutex();
    result._mtx = mtx;
    return result;
  }

  /**
   * Initializes a new robust mutex in the memory provided by the argument
   * pointer.
   */
  @Override
  public void initialize(Pointer mtx) {
    _mtx = mtx;

    Pointer attr = new Memory(32);
    PTHREAD.pthread_mutexattr_init(attr);
    PTHREAD.pthread_mutexattr_setpshared(attr, Pthread.PTHREAD_PROCESS_SHARED);
    PTHREAD.pthread_mutexattr_setrobust(attr, Pthread.PTHREAD_MUTEX_ROBUST);

    if (PTHREAD.pthread_mutex_init(_mtx, attr) != 0) {
      assert (false);
    }
  }

  /**
   * Returns the size of the mutex in memory.
   *
   * @return the size (in bytes) of the mutex
   */
  @Override
  public long size() {
    return getSize();
  }

  /*
   * Returns the size in bytes of the mutex in memory.
   */
  static public long getSize() {
    return 40;
  }

  /**
   * Locks the mutex or blocks until mutex is ready to be locked and then locks it
   * and returns.
   */
  @Override
  public void lock() {
    int r = PTHREAD.pthread_mutex_lock(_mtx);

    if (!setConsistent(r) && r != 0) {
      assert (false);
    }
  }

  /**
   * Attempts to lock the mutex and returns immidiately.
   *
   * @return true on success; false on fail
   */
  @Override
  public boolean tryLock() {
    int r = PTHREAD.pthread_mutex_trylock(_mtx);

    if (r == EBUSY) {
      return false;
    } else if (r == 0 || setConsistent(r)) {
      return true;
    } else {
      assert (false);
      return false;
    }
  }

  /**
   * Unlocks the mutex
   */
  @Override
  public void unlock() {
    if (PTHREAD.pthread_mutex_unlock(_mtx) != 0) {
      assert (false);
    }
  }

  /**
   * Marks the mutex as consistent if the last owner died before unlocking.
   *
   * @param lockResults: result of the last operation on the mutex.
   * @return true if last owner has died; false otherwise.
   */
  private boolean setConsistent(int lockResults) {
    if (lockResults == EOWNERDEAD) {
      if (PTHREAD.pthread_mutex_consistent(_mtx) != 0) {
        assert (false);
      }

      return true;
    } else {
      return false;
    }
  }

  /**
   * Interface for Pthread. Required for JNA.
   */
  private interface Pthread extends Library {
    final int PTHREAD_PROCESS_SHARED = 1;
    final int PTHREAD_MUTEX_ROBUST = 1;

    Pthread INSTANCE = Native.load("pthread", Pthread.class);

    int pthread_mutexattr_init(Pointer attr);

    int pthread_mutexattr_setpshared(Pointer attr, int pshared);

    int pthread_mutexattr_setrobust(Pointer attr, int robustness);

    int pthread_mutex_init(Pointer mutex, Pointer attr);

    int pthread_mutex_lock(Pointer mutex);

    int pthread_mutex_trylock(Pointer mutex);

    int pthread_mutex_unlock(Pointer mutex);

    int pthread_mutex_consistent(Pointer mutex);
  }
}