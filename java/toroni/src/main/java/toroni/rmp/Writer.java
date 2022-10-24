/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import java.util.Optional;

import toroni.rmp.detail.*;
import toroni.traits.RobustMutex;

public class Writer {

  private ByteRingBuffer _ringBuf;
  private ReaderInfo _readerInfo;
  private long _indexMask;

  public Writer(ByteRingBuffer ringBuf, ReaderInfo readerInfo) throws Exception {
    _ringBuf = ringBuf;
    _readerInfo = readerInfo;
    _indexMask = Util.indexMask(ringBuf.getBufSize());

    if (!ringBuf.getInitialized()) {
      throw new Exception("Ring buffer not initialized");
    }
  }

  /**
   * @return the maximal useful message size that can be placed in the ring buffer
   *         taking into account system data that is also placed.
   */
  public long getMaxMessageSize() {
    return _ringBuf.getBufSize() - MessageHeader.size();
  }

  /**
   * Writes a message to the stream.
   * 
   * @param data
   * @param bpHandler
   */
  public void writeEx(byte[] data, BackPressureCallback bpHandler) {
    RobustMutex lock = _ringBuf.getMtx();
    lock.lock();

    try {
      Optional<Long> readBp;
      while (true) {
        readBp = write(data, true, Optional.empty());
        if (readBp.isEmpty()) {
          break;
        }

        _ringBuf.incStatBackPressureCount(1);

        boolean continueRunning = bpHandler.writeOrWait(0, _ringBuf.getFreePos());
        if (!continueRunning) {
          write(data, false, Optional.empty());
          break;
        }
      }
    } finally {
      lock.unlock();
    }
  }

  /**
   * Write to stream if back pressure is not taken into account or doesn't occur.
   * 
   * @param data
   * @param readerBP
   * @param skipConsPos
   * @return Optional.empty() if no backpressure was detected; otherwise, the
   *         reader position that caused backpressure
   */
  private Optional<Long> write(byte[] data, boolean readerBP, Optional<Long> skipConsPos) {
    assert (data.length <= getMaxMessageSize());

    long bufIndex = Util.bufIndex(_indexMask, _ringBuf.getFreePos());
    long lengthToBufEnd = _ringBuf.getBufSize() - bufIndex;

    long bytesToWrite = MessageHeader.size() + data.length;
    boolean addPadding = false;
    boolean addBlank = false;

    if (lengthToBufEnd < MessageHeader.size()) {
      addBlank = true;
      bytesToWrite += lengthToBufEnd;
    } else if (lengthToBufEnd < bytesToWrite) {
      addPadding = true;
      bytesToWrite += lengthToBufEnd;
    }

    if (readerBP) {
      Optional<Long> readerBpPos = detectReaderBackpressure(bytesToWrite, skipConsPos);
      if (readerBpPos.isPresent()) {
        return readerBpPos;
      }
    }

    if (addPadding) {
      // set the header of the padding
      _ringBuf.setByte(bufIndex + MessageHeader.TYPE_OFFSET, MessageHeader.PADDING);
      _ringBuf.setInt(bufIndex + MessageHeader.LENGTH_OFFSET, (int) (lengthToBufEnd - MessageHeader.size()));

      _ringBuf.incFreePos(lengthToBufEnd);
      bufIndex = 0;
    } else if (addBlank) {
      _ringBuf.incFreePos(lengthToBufEnd);
      bufIndex = 0;
    }

    // write the header of the message
    _ringBuf.setByte(bufIndex + MessageHeader.TYPE_OFFSET, MessageHeader.MESSAGE);
    _ringBuf.setInt(bufIndex + MessageHeader.LENGTH_OFFSET, data.length);

    // write the message itself
    _ringBuf.setBytes(bufIndex + MessageHeader.size(), data);

    _ringBuf.incFreePos(MessageHeader.size() + data.length);

    return Optional.empty();
  }

  /**
   * Detects if writing n bytes and ignoring reader positions <= skipConsPos will
   * expire a reader.
   * 
   * @param n
   * @param skipConsPos
   * @return Optional.empty() if no reader will expire; otherwise, the
   *         reader position will expire
   */
  private Optional<Long> detectReaderBackpressure(long n, Optional<Long> skipConsPos) {
    long skipConsPosValue = (skipConsPos.isEmpty() ? 0 : skipConsPos.get());

    short[] minMax = _readerInfo.getActiveRange();
    for (int i = minMax[0]; i < minMax[1]; i++) {
      ReaderInfoInfo readerInfo = _readerInfo.getInfo(i);

      if (!readerInfo.getIsActive()) {
        continue;
      }

      long readerPos = readerInfo.getPosition();

      if (Util.expired(readerPos, _ringBuf.getFreePos(), _ringBuf.getBufSize())) {
        continue;
      }

      if (skipConsPos.isEmpty() || Util.greaterThan(readerPos, skipConsPosValue)) {
        if (Util.expired(readerPos, _ringBuf.getFreePos() + n, _ringBuf.getBufSize())) {
          return Optional.of(readerPos);
        }
      }
    }

    return Optional.empty();
  }

}
