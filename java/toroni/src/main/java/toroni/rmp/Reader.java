/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import com.sun.jna.Pointer;

import toroni.rmp.detail.*;

public class Reader {
  public enum Result {
    SUCCESS, INVALID_POSITION, EXPIRED_POSITION
  };

  public class ResultPosPair {
    public Result res;
    public long pos;

    ResultPosPair(Result _res, long _pos) {
      res = _res;
      pos = _pos;
    }
  }

  private final ByteRingBuffer _ringBuf;
  private final long _indexMask;

  public Reader(ByteRingBuffer ringBuf) throws Exception {
    _ringBuf = ringBuf;
    _indexMask = Util.indexMask(_ringBuf.getBufSize());

    if (!ringBuf.getInitialized()) {
      throw new Exception("Ring buffer not initialized");
    }
  }

  /**
   * Read starting from stream position {@code pos} until the stream end.
   * 
   * @param pos:           Stream position from which to start reading
   * @param readerInfo:    Info corresponding to the reader so that we can update
   *                       the position in the reader info shared memory
   * @param copyConfirmCb: Invoke Copy for every message. If it returns false,
   *                       message is discarded. If it returns true, message
   *                       processing continues. Confirm is called if reader has
   *                       not expired
   * @return INVALID_POSITION if pos is after the stream end. EXPIRED_POSITION if
   *         the ring buffer has been overwritten at stream position pos. SUCCESS
   *         otherwise.
   */
  public ResultPosPair readEx(long pos, ReaderInfoInfo readerInfo, CopyConfirmCallback copyConfirmCb) {
    if (Util.greaterThan(pos, _ringBuf.getFreePos())) {
      return new ResultPosPair(Result.INVALID_POSITION, pos);
    }

    long bposMaxLag = _ringBuf.getBufSize();

    while (pos != _ringBuf.getFreePos()) {
      long bufIndex = Util.bufIndex(_indexMask, pos);

      if (_ringBuf.getBufSize() - bufIndex < MessageHeader.size()) {
        // There is not enough space for a message header till the end of the buffer,
        // skip these positions.
        pos += _ringBuf.getBufSize() - bufIndex;
        continue;
      }

      if (Util.expired(pos, _ringBuf.getFreePos(), _ringBuf.getBufSize())) {
        return new ResultPosPair(Result.EXPIRED_POSITION, pos);
      }

      MessageHeader msgHeaderCpy = Util.readMsgHeader(_ringBuf, bufIndex);
      if (!msgHeaderCpy.valid() || Util.expired(pos, _ringBuf.getFreePos(), _ringBuf.getBufSize())) {
        return new ResultPosPair(Result.EXPIRED_POSITION, pos);
      }

      assert (bufIndex + MessageHeader.size() + msgHeaderCpy.length <= _ringBuf.getBufSize());

      if (msgHeaderCpy.type == MessageHeader.MESSAGE) {
        Pointer data = new Pointer(_ringBuf.BUFFER_ADDRESS + bufIndex + MessageHeader.size());

        if (copyConfirmCb.copy(data, msgHeaderCpy.length)) {
          // Reliable pessimistic expiration check.
          if (Util.expired(pos, _ringBuf.getFreePos(), _ringBuf.getBufSize())) {
            return new ResultPosPair(Result.EXPIRED_POSITION, pos);
          }

          copyConfirmCb.confirm();
        }
      }

      pos += MessageHeader.size() + msgHeaderCpy.length;

      if (pos >= readerInfo.getPosition() + bposMaxLag) {
        readerInfo.setPosition(pos);
      }
    }

    readerInfo.setPosition(pos);

    return new ResultPosPair(Result.SUCCESS, pos);
  }
}
