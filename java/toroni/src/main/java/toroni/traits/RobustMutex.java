/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.traits;

import com.sun.jna.Pointer;

public interface RobustMutex {

  /**
   * Factory method
   * 
   * @return
   */
  RobustMutex create();

  /**
   * Initializes a new robust mutex in the memory provided by the argument
   * pointer.
   */
  void initialize(Pointer mtx);

  /**
   * Returns the size of the mutex in memory.
   * 
   * @return the size (in bytes) of the mutex
   */
  long size();

  /**
   * Locks the mutex or blocks until mutex is ready to be locked and then locks it
   * and returns.
   */
  public void lock();

  /**
   * Attempts to lock the mutex and returns immidiately.
   * 
   * @return true on success; false on fail.
   */
  public boolean tryLock();

  /**
   * Unlocks the mutex.
   */
  public void unlock();

}