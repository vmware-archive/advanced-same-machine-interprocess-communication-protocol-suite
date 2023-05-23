/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.toroni.traits.posix;

import org.junit.jupiter.api.RepeatedTest;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTimeoutPreemptively;

import java.time.Duration;
import java.util.concurrent.atomic.AtomicBoolean;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.vmware.toroni.traits.posix.PthreadRobustMutex;

class PthreadRobustMutexTest {
  private final int TIMEOUT_MS = 100;

  @Test
  void tryLockUnlocked() {
    Pointer mtxPointer = new Memory(PthreadRobustMutex.getSize());
    PthreadRobustMutex mtx = new PthreadRobustMutex();
    mtx.initialize(mtxPointer);

    Thread t = new Thread(new Runnable() {
      @Override
      public void run() {
        mtx.lock();
        mtx.unlock();
      }
    });
    t.start();

    try {
      t.join();
      assertEquals(true, mtx.tryLock());
    } catch (Exception e) {
      assert (false);
    }
  }

  @Test
  @RepeatedTest(100)
  void tryLockDead() {
    Pointer mtxPointer = new Memory(PthreadRobustMutex.getSize());
    PthreadRobustMutex mtx = new PthreadRobustMutex();
    mtx.initialize(mtxPointer);

    Thread t = new Thread(new Runnable() {
      @Override
      public void run() {
        mtx.lock();
      }
    });
    t.start();

    try {
      t.join();

      /**
       * After join sometimes the mutex is still busy for some time.
       */
      assertTimeoutPreemptively(Duration.ofMillis(TIMEOUT_MS), () -> {
        while (!mtx.tryLock())
          ;
      });

      return;
    } catch (Exception e) {
      assert (false);
    }
  }

  @Test
  @RepeatedTest(100)
  void lockLocked() {
    Pointer mtxPointer = new Memory(PthreadRobustMutex.getSize());
    PthreadRobustMutex mtx = new PthreadRobustMutex();
    mtx.initialize(mtxPointer);

    AtomicBoolean locked1 = new AtomicBoolean(false);
    AtomicBoolean finished1 = new AtomicBoolean(false);
    AtomicBoolean locked2 = new AtomicBoolean(false);

    Thread t1 = new Thread(new Runnable() {
      @Override
      public void run() {
        mtx.lock();
        locked1.compareAndExchange(false, true);
        while (!finished1.get())
          ;
      }
    });
    t1.start();

    while (!locked1.get())
      ;

    Thread t2 = new Thread(new Runnable() {
      @Override
      public void run() {
        mtx.lock();
        locked2.compareAndExchange(false, true);
      }
    });
    t2.start();

    try {
      assertTimeoutPreemptively(Duration.ofMillis(TIMEOUT_MS), () -> {
        while (locked2.get())
          ;
      });

      finished1.set(true);

      assertTimeoutPreemptively(Duration.ofMillis(TIMEOUT_MS), () -> {
        while (!locked2.get())
          ;
      });

      t1.join();
      t2.join();
    } catch (Exception e) {
      assert (false);
    }
  }

  @Test
  void size() {
    Pointer mtxPointer = new Memory(PthreadRobustMutex.getSize());
    PthreadRobustMutex mtx = new PthreadRobustMutex();
    mtx.initialize(mtxPointer);

    assertEquals(40, mtx.size());
  }
}
