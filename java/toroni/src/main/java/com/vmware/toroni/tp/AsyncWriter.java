/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.tp;

import java.util.ArrayList;

import com.vmware.toroni.rmp.BackPressureCallback;
import com.vmware.toroni.rmp.ByteRingBuffer;
import com.vmware.toroni.rmp.Writer;
import com.vmware.toroni.tp.detail.TopicMsgSerializer;

/**
 * Topic Protocol (TP) async message writer
 */
public class AsyncWriter {

  public static interface DrainMsgFn {
    public ArrayList<byte[]> run();
  }

  public static interface EnqueueMsgFn {
    public boolean run(byte[] topicMsg);
  }

  public static interface EnqueueWorkFn {
    public void run(Runnable workFn);
  }

  private ByteRingBuffer _ringBuf;
  private ReaderInfo _readerInfo;
  private Writer _rbWriter;
  private EnqueueMsgFn _enqueueMsgFn;
  private DrainMsgFn _drainMsgFn;
  private EnqueueWorkFn _enqueueWorkFn;
  private BackPressureCallback _backPressureFn;
  private Runnable _notifyAllReadersFn;

  private AsyncWriter(ByteRingBuffer ringBuf, ReaderInfo readerInfo, EnqueueMsgFn enqueueMsgFn,
      DrainMsgFn drainMsgFn, EnqueueWorkFn enqueueWorkFn, BackPressureCallback backPressureFn,
      Runnable notifyAllReadersFn) {
    _ringBuf = ringBuf;
    _readerInfo = readerInfo;
    try {
      _rbWriter = new Writer(_ringBuf, _readerInfo.rmpReaderInfo);
    } catch (Exception e) {
      throw new Error(e);
    }
    _enqueueMsgFn = enqueueMsgFn;
    _drainMsgFn = drainMsgFn;
    _enqueueWorkFn = enqueueWorkFn;
    _backPressureFn = backPressureFn;
    _notifyAllReadersFn = notifyAllReadersFn;
  }

  /**
   * Create a new async topic message writer.
   * 
   * @param ringBuf
   * @param readerInfo
   * @param enqueueMsgFn:      thread-safe function to enqueue messages
   * @param drainMsgFn:        thread-safe function to dequeue all messages
   * @param enqueueWorkFn:     thread-safe function to enqueue work
   * @param backPressureFn:    invoked when backpressure is detected
   * @param notifyAllReadersFn invoked to send notification to all readers to
   *                           start reading
   * @return async topic message writer
   */
  public static AsyncWriter create(ByteRingBuffer ringBuf, ReaderInfo readerInfo,
      EnqueueMsgFn enqueueMsgFn, DrainMsgFn drainMsgFn, EnqueueWorkFn enqueueWorkFn,
      BackPressureCallback backPressureFn, Runnable notifyAllReadersFn) {
    return new AsyncWriter(ringBuf, readerInfo,
        enqueueMsgFn, drainMsgFn, enqueueWorkFn, backPressureFn, notifyAllReadersFn);
  }

  /**
   * Create a topic message.
   * 
   * @param channelName
   * @param msg
   * @param postToDescendants
   * @return a topic message in the form of ArrayList<Byte>
   * @throws Exception if message data is too big
   */
  public byte[] createMessage(String channelName, byte[] msg,
      boolean postToDescendants) throws Exception {
    assert (msg.length != 0);

    int topicMsgLen = TopicMsgSerializer.sizeOf(channelName, msg.length);

    if (topicMsgLen > _rbWriter.getMaxMessageSize()) {
      throw new Exception("Message size exceeds RingBuffer size");
    }

    byte[] rbMsg = new byte[topicMsgLen];
    TopicMsgSerializer.serialize(rbMsg, _readerInfo.getReaderGen(),
        postToDescendants, channelName, msg);

    return rbMsg;
  }

  /**
   * Posts a message and returns immediatelly. Invokes {@code _enqueueWorkFn} with
   * {@code procWriter} if none is running.
   * 
   * @param rbMsg
   */
  public void post(byte[] rbMsg) {
    assert (rbMsg.length < _rbWriter.getMaxMessageSize());

    if (_enqueueMsgFn.run(rbMsg)) {
      _enqueueWorkFn.run(new Runnable() {

        @Override
        public void run() {
          procWriter();
        }

      });
    }
  }

  /**
   * The actual writing to the ring buffer.
   */
  public void procWriter() {
    BackPressureCallback bpWrapper = new BackPressureCallback() {

      @Override
      public boolean writeOrWait(long bpPos, long freePos) {
        // Notify readers on backpressure
        _ringBuf.incStatNotificationCount(1);
        _notifyAllReadersFn.run();

        // Invoke backpressure callback
        return _backPressureFn.writeOrWait(bpPos, freePos);
      }

    };

    while (true) {
      ArrayList<byte[]> queueCopy = _drainMsgFn.run();

      if (queueCopy.isEmpty()) {
        // Notify when the loop ends
        _ringBuf.incStatNotificationCount(1);
        _notifyAllReadersFn.run();
        break;
      }

      queueCopy.forEach((msg) -> {
        _rbWriter.writeEx(msg, bpWrapper);
      });
    }
  }

}
