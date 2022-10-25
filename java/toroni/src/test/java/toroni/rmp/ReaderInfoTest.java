/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.atomic.AtomicInteger;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import com.sun.jna.Pointer;
import com.sun.jna.Memory;

import toroni.traits.PthreadRobustMutex;

class ReaderInfoTest {
  private short maxReaders;
  private ReaderInfo readerInfo;

  @BeforeEach
  void init() {
    maxReaders = 3;
    Pointer readerInfoPointer = new Memory(ReaderInfo.size(maxReaders, PthreadRobustMutex.getSize()));

    readerInfo = new ReaderInfo(readerInfoPointer, maxReaders, new PthreadRobustMutex());
    readerInfo.initialize();
  }

  @Test
  void initialization() {
    assertTrue(readerInfo.getInitialized());
    assertEquals(0, readerInfo.getStatExpiredReaders());

    short[] minMax = readerInfo.getActiveRange();
    assertTrue(minMax[0] >= minMax[1]);
  }

  @Test
  void activateDeactivateRange() {
    short[] minMax;

    for (int i = 0; i < maxReaders; i++) {
      readerInfo.activate(i, i + 1);
      minMax = readerInfo.getActiveRange();
      assertTrue(readerInfo.getInfo(i).getIsActive());
      assertEquals(i + 1, readerInfo.getInfo(i).getPosition());
      assertEquals(0, minMax[0]);
      assertEquals(i + 1, minMax[1]);
    }

    readerInfo.deactivate(1);
    minMax = readerInfo.getActiveRange();
    assertEquals(0, minMax[0]);
    assertEquals(3, minMax[1]);

    readerInfo.deactivate(0);
    minMax = readerInfo.getActiveRange();
    assertEquals(2, minMax[0]);
    assertEquals(3, minMax[1]);

    readerInfo.deactivate(2);
    minMax = readerInfo.getActiveRange();
    assertTrue(minMax[0] >= minMax[1]);
  }

  @Test
  void allocFreeNoFree() {
    int readers = 5;

    AtomicInteger allocCount = new AtomicInteger(0);
    AtomicInteger allocFail = new AtomicInteger(0);
    AtomicInteger allocSuccess = new AtomicInteger(0);

    Runnable proc = new Runnable() {
      @Override
      public void run() {
        int readerId = readerInfo.alloc();

        allocCount.incrementAndGet();
        while (allocCount.get() != readers)
          ;

        if (readerId == -1) {
          allocFail.incrementAndGet();
        } else {
          allocSuccess.incrementAndGet();
        }

        readerInfo.free(readerId);
      }
    };

    Thread[] threads = new Thread[readers];
    for (int i = 0; i < readers; i++) {
      threads[i] = new Thread(proc);
      threads[i].start();
    }

    try {
      for (int i = 0; i < readers; i++) {
        threads[i].join();
      }

      assertEquals(maxReaders, allocSuccess.get());
      assertEquals(readers - maxReaders, allocFail.get());

      short[] minMax = readerInfo.getActiveRange();
      assertTrue(minMax[0] >= minMax[1]);

      int readerId = readerInfo.alloc();
      assertEquals(0, readerId);
    } catch (Exception e) {
      assert (false);
    }
  }

  @Test
  void allocDead() {
    Runnable proc = new Runnable() {
      @Override
      public void run() {
        int readerId = readerInfo.alloc();
        readerInfo.activate(readerId, 0);

        assertNotEquals(ReaderInfo.INVALID_READER_ID, readerId);
        assertEquals(true, readerInfo.getInfo(readerId).getIsActive());

        readerInfo.free(readerId);
      }
    };

    for (int i = 0; i < 3 * maxReaders; i++) {
      Thread t = new Thread(proc);
      t.start();

      try {
        t.join();
      } catch (Exception e) {
        assert (false);
      }
    }
  }
}
