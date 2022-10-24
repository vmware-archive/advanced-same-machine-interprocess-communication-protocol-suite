/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
 
package toroni.tp;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

class SerialWorkItemQueueTest {

  @Test
  void orderSingleProducer() {
    ArrayList<Integer> arrList = new ArrayList<>();
    SerialWorkItemQueue wq = SerialWorkItemQueue.start();

    for (int i = 0; i < 5; i++) {
      final int currI = i;

      wq.enqueue(new Runnable() {

        @Override
        public void run() {
          arrList.add(currI);
        }

      });
    }

    try {
      Thread.sleep(1);
    } catch (Exception e) {
      throw new Error(e);
    }

    for (int i = 5; i < 10; i++) {
      final int currI = i;

      wq.enqueue(new Runnable() {

        @Override
        public void run() {
          arrList.add(currI);
        }

      });
    }

    wq.stop(SerialWorkItemQueue.StopPolicy.IF_EMPTY);

    Assertions.assertArrayEquals(new Integer[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, arrList.toArray());
  }

  @Test
  void countManyProducers() {
    ArrayList<Thread> pushers = new ArrayList<>();
    ArrayList<Integer> arrList = new ArrayList<>();

    SerialWorkItemQueue wq = SerialWorkItemQueue.start();

    for (int i = 0; i < 25; i++) {
      final int currI = i;

      pushers.add(new Thread(new Runnable() {

        @Override
        public void run() {
          wq.enqueue(new Runnable() {

            @Override
            public void run() {
              arrList.add(currI);
            }

          });
        }

      }));

      pushers.get(pushers.size() - 1).start();
    }

    pushers.forEach((pusher) -> {
      try {
        pusher.join();
      } catch (Exception e) {
        throw new Error(e);
      }
    });

    wq.stop(SerialWorkItemQueue.StopPolicy.IF_EMPTY);

    assertEquals(25, arrList.size());
  }

}
