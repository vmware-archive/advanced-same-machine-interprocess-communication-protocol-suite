/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.traits.concurrent;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * A work item queue that executes them serially, i.e. in the same order.
 */
public class SerialWorkItemQueue {

  public enum StopPolicy {
    IF_EMPTY, ALWAYS
  };

  private Thread _thread;
  private ArrayList<Runnable> _workItemQueue;
  private AtomicBoolean _stop;
  private AtomicBoolean _stopPolicy; // false - IF_EMPTY, true - ALWAYS

  private SerialWorkItemQueue() {
    _stop = new AtomicBoolean(false);
    _stopPolicy = new AtomicBoolean();
    _workItemQueue = new ArrayList<>();
  }

  /**
   * Starts a new SerialWorkItemQueue. It also runs a separate thread that is
   * responsible for executing the items in the queue.
   * 
   * @return the new running SerialWorkItemQueue
   */
  public static SerialWorkItemQueue start() {
    SerialWorkItemQueue result = new SerialWorkItemQueue();

    result._thread = new Thread(new Runnable() {

      @Override
      public void run() {
        result.run();
      }

    });
    result._thread.start();

    return result;
  }

  /**
   * Should be called when the SerialWorkItemQueue is not going to be used
   * anymore.
   */
  public void destroy() {
    stop(StopPolicy.ALWAYS);
  }

  /**
   * Pushes a new item in the queue. Then notifies the thread processing the queue
   * that a new item has appeared.
   * 
   * @param wi
   */
  public synchronized void enqueue(Runnable wi) {
    _workItemQueue.add(wi);
    notify();
  }

  /**
   * Stops the thread processing the queue, but waits for it to finish.
   * 
   * @param stopPolicy: whether to stop the thread regardless queue is empty
   */
  public void stop(StopPolicy stopPolicy) {
    if (!_thread.isAlive()) {
      return;
    }

    _stop.set(true);
    _stopPolicy.set(stopPolicy == StopPolicy.ALWAYS);

    synchronized (this) {
      notify();
    }

    try {
      _thread.join();
    } catch (Exception e) {
      throw new Error(e);
    }
  }

  /**
   * Runs items in the queue serially.
   */
  private void run() {
    while (true) {
      ArrayList<Runnable> queueCopy;

      synchronized (this) {
        try {
          while (_workItemQueue.isEmpty() && !_stop.get())
            wait();
        } catch (Exception e) {
          throw new Error(e);
        }

        if (_stop.get()) {
          if (_stopPolicy.get() || _workItemQueue.isEmpty()) {
            break;
          }
        }

        queueCopy = new ArrayList<>(_workItemQueue);
        _workItemQueue.clear();
      }

      try {
        queueCopy.forEach((wi) -> wi.run());
      } catch (Exception e) {

      }
    }
  }

}
