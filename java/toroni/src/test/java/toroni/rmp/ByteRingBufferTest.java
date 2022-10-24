/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;

import toroni.traits.PthreadRobustMutex;
import toroni.traits.RobustMutex;

class ByteRingBufferTest {

  private final int MUTEX_SIZE_BYTES = 40;

  private long bufSize;
  private ByteRingBuffer ringBuf;

  @BeforeEach
  void init() {
    bufSize = 1024;

    Pointer mtxPointer = new Memory(MUTEX_SIZE_BYTES);
    RobustMutex mtx = new PthreadRobustMutex(mtxPointer);
    mtx.initialize();

    Pointer ringBufPointer = new Memory(ByteRingBuffer.size(bufSize, PthreadRobustMutex.getSize()));
    ringBuf = new ByteRingBuffer(ringBufPointer, bufSize, mtx);

    ringBuf.initialize();
  }

  @Test
  void initialized() {
    assertEquals(true, ringBuf.getInitialized());
    assertEquals(bufSize, ringBuf.getBufSize());
    assertEquals(0, ringBuf.getFreePos());
    assertEquals(0, ringBuf.getStatBackPressureCount());
    assertEquals(0, ringBuf.getStatNotificationCount());
  }

  @Test
  void changeVariables() {
    ringBuf.setFreePos(1);
    assertEquals(1, ringBuf.getFreePos());

    ringBuf.setStatBackPressureCount(2);
    assertEquals(2, ringBuf.getStatBackPressureCount());

    ringBuf.setStatNotificationCount(3);
    assertEquals(3, ringBuf.getStatNotificationCount());
  }

  @Test
  void setIndexAndGetIndex() {
    for (int i = 0; i < bufSize; i++) {
      ringBuf.setByte(i, (byte) (i % 256));
    }

    for (int i = 0; i < bufSize; i++) {
      assertEquals((byte) (i % 256), ringBuf.getByte(i));
    }
  }

}
