/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.tp;

import com.sun.jna.Pointer;
import com.sun.jna.Memory;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.nio.ByteBuffer;
import java.util.ArrayList;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.InOrder;
import org.mockito.Mockito;

import toroni.rmp.BackPressureCallback;
import toroni.rmp.ByteRingBuffer;
import toroni.rmp.Writer;
import toroni.rmp.detail.MessageHeader;
import toroni.tp.Reader.ChannelReaderEventCallback;
import toroni.tp.Reader.ChannelReaderEventType;
import toroni.traits.PthreadRobustMutex;
import toroni.traits.RobustMutex;

class ReaderTest {

  private final int MUTEX_SIZE_BYTES = 40;

  private Reader reader;
  private Writer writer;
  private ChannelReaderEventCallback mockEventCallback;
  private ByteRingBuffer ringBuf;
  private ReaderInfo readerInfo;
  private ArrayList<Runnable> sqReader, sqRmp;

  ByteRingBuffer initRingBuf() {
    long bufSize = 1024;

    Pointer mtxPointer = new Memory(MUTEX_SIZE_BYTES);
    RobustMutex mtx = new PthreadRobustMutex(mtxPointer);
    mtx.initialize();

    Pointer ringBufPointer = new Memory(ByteRingBuffer.size(bufSize, PthreadRobustMutex.getSize()));
    ByteRingBuffer ringBuf = new ByteRingBuffer(ringBufPointer, bufSize, mtx);

    ringBuf.initialize();
    return ringBuf;
  }

  ReaderInfo initReaderInfo() {
    short maxReaders = 3;

    Pointer[] mtxPointers = {
        new Memory(MUTEX_SIZE_BYTES),
        new Memory(MUTEX_SIZE_BYTES),
        new Memory(MUTEX_SIZE_BYTES)
    };

    RobustMutex[] locks = new PthreadRobustMutex[maxReaders];
    for (int i = 0; i < maxReaders; i++) {
      locks[i] = new PthreadRobustMutex(mtxPointers[i]);
      locks[i].initialize();
    }

    Pointer readerInfoPointer = new Memory(ReaderInfo.size(maxReaders, MUTEX_SIZE_BYTES));

    ReaderInfo readerInfo = new ReaderInfo(readerInfoPointer, maxReaders, locks);
    readerInfo.initialize();

    return readerInfo;
  }

  @BeforeEach
  void init() {
    ringBuf = initRingBuf();
    readerInfo = initReaderInfo();
    sqReader = new ArrayList<>();
    sqRmp = new ArrayList<>();
    mockEventCallback = Mockito.mock(ChannelReaderEventCallback.class);

    reader = Reader.create(ringBuf, readerInfo, new Reader.EnqueueSerialFn() {

      @Override
      public void run(Runnable fn) {
        sqReader.add(fn);
      }

    }, new Reader.EnqueueSerialFn() {

      @Override
      public void run(Runnable fn) {
        sqRmp.add(fn);
      }

    }, mockEventCallback);

    try {
      writer = new Writer(ringBuf, readerInfo.rmpReaderInfo);
    } catch (Exception e) {
      assert (false);
    }
  }

  void runQueue(ArrayList<Runnable> q) {
    for (Runnable i : q) {
      i.run();
    }
    q.clear();
  }

  void postMessage(String topic, String data, boolean postToDescendants) {
    AsyncWriter.EnqueueMsgFn mockEnqueueMsgFn = Mockito.mock(AsyncWriter.EnqueueMsgFn.class);
    AsyncWriter.DrainMsgFn mockDrainMsgFn = Mockito.mock(AsyncWriter.DrainMsgFn.class);
    Runnable mockNotifyFn = Mockito.mock(Runnable.class);
    BackPressureCallback mockBpFn = Mockito.mock(BackPressureCallback.class);
    AsyncWriter writer = AsyncWriter.create(ringBuf, readerInfo,
        mockEnqueueMsgFn, mockDrainMsgFn, new AsyncWriter.EnqueueWorkFn() {

          @Override
          public void run(Runnable workFn) {
            workFn.run();
          }

        }, mockBpFn, mockNotifyFn);

    byte[] msg;
    try {
      msg = writer.createMessage(topic, data.getBytes(), postToDescendants);
    } catch (Exception e) {
      throw new Error(e);
    }

    ArrayList<byte[]> emptyQueue = new ArrayList<>();

    ArrayList<byte[]> queueOneEl = new ArrayList<>();
    queueOneEl.add(msg);

    when(mockEnqueueMsgFn.run(any())).thenReturn(true);
    when(mockDrainMsgFn.run()).thenReturn(queueOneEl, emptyQueue);

    writer.post(msg);

    InOrder orderVerifier = Mockito.inOrder(mockEnqueueMsgFn, mockDrainMsgFn, mockNotifyFn, mockBpFn);
    orderVerifier.verify(mockNotifyFn, times(1)).run();
    orderVerifier.verify(mockBpFn, times(0)).writeOrWait(anyLong(), anyLong());
  }

  int writeBigData(BackPressureCallback bpHandler) {
    int len = (int) (ringBuf.getBufSize() - MessageHeader.size() - 1);
    byte[] bigData = new byte[len];
    writer.writeEx(bigData, bpHandler);
    return len;
  }

  @Test
  void firstLastReader() {
    ChannelReader.Handler mockChannelHandler = Mockito.mock(ChannelReader.Handler.class);

    ChannelReader channelReader = reader.createChannelReader("ch",
        mockChannelHandler, true);
    runQueue(sqReader);
    assertTrue(readerInfo.rmpReaderInfo.getInfo(0).getIsActive());

    reader.closeChannelReader(channelReader);
    runQueue(sqReader);
    assertFalse(readerInfo.rmpReaderInfo.getInfo(0).getIsActive());

    InOrder orderVerifier = Mockito.inOrder(mockEventCallback);
    orderVerifier.verify(mockEventCallback).run(ChannelReaderEventType.FIRST_CHANNEL_READER_CREATED);
    orderVerifier.verify(mockEventCallback).run(ChannelReaderEventType.LAST_CHANNEL_READER_CLOSED);
  }

  @Test
  void topicMatch() {
    // channel close, not called
    ChannelReader.Handler h1 = Mockito.mock(ChannelReader.Handler.class);
    ChannelReader channelReader = reader.createChannelReader("ch", h1, true);
    reader.closeChannelReader(channelReader);

    // channel matching
    ChannelReader.Handler h2 = Mockito.mock(ChannelReader.Handler.class);
    reader.createChannelReader("ch", h2, false);

    // channel not matching
    ChannelReader.Handler h3 = Mockito.mock(ChannelReader.Handler.class);
    reader.createChannelReader("h", h3, false);

    // channel matching, handle descendants
    ChannelReader.Handler h4 = Mockito.mock(ChannelReader.Handler.class);
    reader.createChannelReader("c", h4, true);

    runQueue(sqReader);
    postMessage("ch", "data", true);

    // channel created after message was posted, not recieving
    ChannelReader.Handler h5 = Mockito.mock(ChannelReader.Handler.class);
    reader.createChannelReader("h5", h5, false);

    reader.run();
    runQueue(sqReader);
    runQueue(sqRmp);

    verify(h1, times(0)).run(any());
    verify(h2).run(ByteBuffer.wrap("data".getBytes()));
    verify(h3, times(0)).run(any());
    verify(h4).run(any());
    verify(h5, times(0)).run(any());

  }

  @Test
  void expired() {
    ChannelReader.Handler h2 = Mockito.mock(ChannelReader.Handler.class);
    reader.createChannelReader("ch", h2, false);

    runQueue(sqReader);

    BackPressureCallback noBPHandler = new BackPressureCallback() {

      @Override
      public boolean writeOrWait(long bpPos, long freePos) {
        return false;
      }

    };
    writeBigData(noBPHandler);
    writeBigData(noBPHandler);

    reader.run();
    runQueue(sqReader);
    runQueue(sqRmp);

    verify(mockEventCallback, times(2)).run(any());
    verify(mockEventCallback).run(ChannelReaderEventType.ALL_CHANNEL_READERS_EXPIRES);

    verify(h2, times(0)).run(any());
  }

  @Test
  void runWithoutReaders() {
    reader.run();

    assertEquals(1, sqReader.size());
    assertEquals(0, sqRmp.size());
  }

}
