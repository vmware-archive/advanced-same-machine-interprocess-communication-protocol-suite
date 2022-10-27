/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.toroni.rmp;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.vmware.toroni.rmp.BackPressureCallback;
import com.vmware.toroni.rmp.ByteRingBuffer;
import com.vmware.toroni.rmp.CopyConfirmCallback;
import com.vmware.toroni.rmp.Reader;
import com.vmware.toroni.rmp.ReaderInfo;
import com.vmware.toroni.rmp.ReaderWithBackpressure;
import com.vmware.toroni.rmp.Writer;
import com.vmware.toroni.rmp.detail.MessageHeader;
import com.vmware.toroni.traits.posix.PthreadRobustMutex;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

class ReaderWriterTest {
  private Writer writer;
  private ReaderWithBackpressure readerBP;
  private BackPressureCallback noBPHandler;
  private ReadIntCopyConfirmHandler readHandler;
  private ByteRingBuffer ringBuf;
  private ReaderInfo readerInfo;
  private BackPressureCallback mockBPHandler;

  private int maxIntMsg;

  private class ReadIntCopyConfirmHandler implements CopyConfirmCallback {
    private ByteRingBuffer _ringBuf;
    private int _tmpData;
    public ArrayList<Integer> data = new ArrayList<Integer>();

    public ReadIntCopyConfirmHandler(ByteRingBuffer ringBuf) {
      _ringBuf = ringBuf;
    }

    @Override
    public boolean copy(long index, int length) {
      ByteBuffer bb = ByteBuffer.allocate(length);
      _ringBuf.getBytes(index, length, bb.array());
      bb.order(ByteOrder.LITTLE_ENDIAN);
      _tmpData = bb.getInt();
      return true;
    }

    @Override
    public void confirm() {
      data.add(_tmpData);
    }
  }

  void initRingBuf() {
    long bufSize = 1024;

    Pointer ringBufPointer = new Memory(ByteRingBuffer.size(bufSize, PthreadRobustMutex.getSize()));
    ringBuf = new ByteRingBuffer(ringBufPointer, bufSize, new PthreadRobustMutex());

    ringBuf.initialize();
  }

  void initReaderInfo() {
    short maxReaders = 3;

    Pointer readerInfoPointer = new Memory(ReaderInfo.size(maxReaders, PthreadRobustMutex.getSize()));

    readerInfo = new ReaderInfo(readerInfoPointer, maxReaders, new PthreadRobustMutex());
    readerInfo.initialize();
  }

  @BeforeEach
  void init() {
    initRingBuf();
    initReaderInfo();

    try {
      writer = new Writer(ringBuf, readerInfo);
      readerBP = new ReaderWithBackpressure(ringBuf, readerInfo);
    } catch (Exception e) {
      assert (false);
    }

    noBPHandler = new BackPressureCallback() {
      @Override
      public boolean writeOrWait(long bpPos, long freePos) {
        return false;
      }
    };

    mockBPHandler = Mockito.mock(BackPressureCallback.class);

    readHandler = new ReadIntCopyConfirmHandler(ringBuf);

    maxIntMsg = (int) (ringBuf.getBufSize() / (MessageHeader.size() + Integer.BYTES));
  }

  void writeInt(int v) {
    // convert the int to byte array
    byte[] data = { (byte) ((v >> 0) & 0xff), (byte) ((v >> 8) & 0xff),
        (byte) ((v >> 16) & 0xff), (byte) ((v >> 24) & 0xff) };

    writer.writeEx(data, noBPHandler);
  }

  int writeBigData(BackPressureCallback bpHandler) {
    int len = (int) (ringBuf.getBufSize() - MessageHeader.size() - 1);
    byte[] bigData = new byte[len];
    writer.writeEx(bigData, bpHandler);
    return len;
  }

  @Test
  void scratchReaderBPReadEmpty() {
    readerBP.activate();
    readerBP.readEx(readHandler);

    assertTrue(readHandler.data.isEmpty());
  }

  @Test
  void scratchWrite2Read2ReadEmpty() {
    readerBP.activate();

    writeInt(1);
    writeInt(2);

    assertEquals(Reader.Result.SUCCESS, readerBP.readEx(readHandler));
    Assertions.assertArrayEquals(
        new int[] { 1, 2 }, readHandler.data.stream().mapToInt(i -> i).toArray());

    readHandler.data.clear();
    assertEquals(Reader.Result.SUCCESS, readerBP.readEx(readHandler));
    Assertions.assertArrayEquals(
        new int[] {}, readHandler.data.stream().mapToInt(i -> i).toArray());
  }

  @Test
  void scratchReadBufLen() {
    readerBP.activate();

    for (int i = 1; i <= maxIntMsg; i++) {
      writeInt(i);
    }

    assertEquals(Reader.Result.SUCCESS, readerBP.readEx(readHandler));
    assertEquals(maxIntMsg, readHandler.data.size());
  }

  @Test
  void scratchReadExpired() {
    readerBP.activate();

    for (int i = 1; i <= maxIntMsg + 1; i++) {
      writeInt(i);
    }

    assertEquals(Reader.Result.EXPIRED_POSITION, readerBP.readEx(readHandler));
    assertEquals(0, readHandler.data.size());
    assertEquals(1, readerInfo.getStatExpiredReaders());
  }

  @Test
  void someReadBufLen() {
    writeInt(0);

    readerBP.activate();

    for (int i = 1; i <= maxIntMsg - 1; i++) {
      writeInt(i);
    }

    assertEquals(Reader.Result.SUCCESS, readerBP.readEx(readHandler));
    assertEquals(maxIntMsg - 1, readHandler.data.size());
  }

  @Test
  void writeReadNoSpaceForHeader() {
    writeBigData(noBPHandler);

    readerBP.activate();

    writeInt(10);

    assertEquals(Reader.Result.SUCCESS, readerBP.readEx(readHandler));
    Assertions.assertArrayEquals(
        new int[] { 10 }, readHandler.data.stream().mapToInt(i -> i).toArray());
  }

  @Test
  void tooManyBytesToWrite() {
    // With some data in the ring buf and large enough message,
    // padding is aplied and the total number of written bytes is
    // larger than the size of the buffer

    writeInt(10);
    writeBigData(noBPHandler);
  }

  @Test
  void writeBackpressure() {
    readerBP.activate();

    writeBigData(noBPHandler);

    when(mockBPHandler.writeOrWait(anyLong(), anyLong())).thenReturn(false);

    // bp dectected overwrite
    writeBigData(mockBPHandler);
    assertEquals(1, ringBuf.getStatBackPressureCount());

    // skip already expired
    writeBigData(mockBPHandler);
    assertEquals(1, ringBuf.getStatBackPressureCount());
  }

  @Test
  void writeNoActiveReaderNoBackpressure() {
    verify(mockBPHandler, times(0)).writeOrWait(anyLong(), anyLong());

    writeBigData(mockBPHandler);
    writeBigData(mockBPHandler);

    assertEquals(0, ringBuf.getStatBackPressureCount());
  }

  @Test
  void readDenyCopyConfirm() {
    class DenyCopyConfirmHandler implements CopyConfirmCallback {

      public int copyCount = 0;
      public int confirmCount = 0;

      @Override
      public boolean copy(long index, int dataLength) {
        copyCount += dataLength;
        return false;
      }

      @Override
      public void confirm() {
        confirmCount += 1;
      }
    }

    readerBP.activate();

    DenyCopyConfirmHandler dcch = new DenyCopyConfirmHandler();
    int bigDataSize = writeBigData(noBPHandler);

    assertEquals(Reader.Result.SUCCESS, readerBP.readEx(dcch));
    assertEquals(bigDataSize, dcch.copyCount);
    assertEquals(ringBuf.getFreePos(), readerBP.pos());
    assertEquals(0, dcch.confirmCount);
  }

  @Test
  void writeDieWriteDeathTest() {
    readerBP.activate();
    writeInt(20);

    Thread t = new Thread(new Runnable() {
      @Override
      public void run() {
        writeBigData(new BackPressureCallback() {
          @Override
          public boolean writeOrWait(long bpPos, long freePos) {
            throw new Error();
          }
        });
      }
    });

    t.start();
    try {
      t.join();
    } catch (Exception e) {
      assert (false);
    }

    writeInt(30);
    assertEquals(Reader.Result.SUCCESS, readerBP.readEx(readHandler));
    Assertions.assertArrayEquals(
        new int[] { 20, 30 },
        readHandler.data.stream().mapToInt(i -> i).toArray());
  }

  @Test
  void readInactiveDeathTest() {
    Assertions.assertThrows(AssertionError.class,
        () -> {
          readerBP.readEx(readHandler);
        });
  }

  @Test
  void writeBiggerMessageDeathTest() {
    byte[] biggerMessage = new byte[(int) (writer.getMaxMessageSize() + 1)];

    Assertions.assertThrows(AssertionError.class, () -> {
      writer.writeEx(biggerMessage, noBPHandler);
    });
  }
}
