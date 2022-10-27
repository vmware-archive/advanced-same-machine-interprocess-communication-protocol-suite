/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.traits.concurrent;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

class MpscMessageQueueTest {
  private MpscMessageQueue<Integer> mq;

  @BeforeEach
  void init() {
    mq = new MpscMessageQueue<Integer>();
  }

  @Test
  void enqueueSingleDrainer() {
    assertEquals(true, mq.enqueue(1));
    assertEquals(false, mq.enqueue(2));
    assertEquals(false, mq.enqueue(3));
  }

  @Test
  void drainEmpty() {
    Assertions.assertArrayEquals(new Integer[] {}, mq.drain().toArray());
    Assertions.assertArrayEquals(new Integer[] {}, mq.drain().toArray());
  }

  @Test
  void enqueueDrainControlAndData() {
    assertEquals(true, mq.enqueue(1));
    assertEquals(false, mq.enqueue(2));
    Assertions.assertArrayEquals(new Integer[] { 1, 2 }, mq.drain().toArray());
    assertEquals(false, mq.enqueue(3));
    Assertions.assertArrayEquals(new Integer[] { 3 }, mq.drain().toArray());
    Assertions.assertArrayEquals(new Integer[] {}, mq.drain().toArray());
    assertEquals(true, mq.enqueue(4));
  }
}
