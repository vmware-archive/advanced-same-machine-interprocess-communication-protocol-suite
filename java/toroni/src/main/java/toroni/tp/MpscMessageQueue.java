/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.tp;

import java.util.ArrayList;

/**
 * A thread-safe multi-producer, single-consumer queue faciliating the lifecycle
 * of one drainer and multiple enqueuer threads.
 */
public class MpscMessageQueue<T> {

  private ArrayList<T> _queue;
  private boolean _startDrainer;

  public MpscMessageQueue() {
    _queue = new ArrayList<T>();
    _startDrainer = true;
  }

  /**
   * Enqueues a message.
   * 
   * @param msg
   * @return true if a new drainer thread should be started, false otherwise
   */
  public synchronized boolean enqueue(T msg) {
    boolean result = _startDrainer;
    _startDrainer = false; // do not start a new drainer thread on the next call of enqueue
    _queue.add(msg);
    return result;
  }

  /**
   * Drain all messages from the queue.
   * 
   * @return the messages, if empty the drainer thread must exit and next call to
   *         enqueue should return true, otherwise the drainer will continue
   *         duty-cycle and next call to enqueue will return false
   */
  public synchronized ArrayList<T> drain() {
    ArrayList<T> result = new ArrayList<T>(_queue);
    _queue.clear();
    _startDrainer = result.isEmpty(); // result is empty - the drainer thread should exit
    return result;
  }

}
