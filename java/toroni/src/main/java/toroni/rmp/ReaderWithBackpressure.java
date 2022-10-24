/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

public class ReaderWithBackpressure {
  private ByteRingBuffer _ringBuf;
  private ReaderInfo _readerInfo;
  private Reader _reader;
  private int _procReaderId;
  private ReaderInfoInfo _info;
  private long _readerPos;

  public ReaderWithBackpressure(ByteRingBuffer ringBuf, ReaderInfo readerInfo) throws Exception {
    _ringBuf = ringBuf;
    _readerInfo = readerInfo;
    try {
      _reader = new Reader(_ringBuf);
    } catch (Exception e) {
      throw e;
    }

    if (!_readerInfo.getInitialized()) {
      throw new Exception("RMP reader info not initialized");
    }

    _procReaderId = _readerInfo.alloc();
    if (_procReaderId == ReaderInfo.INVALID_READER_ID) {
      throw new Exception("Unable to allocate proc reader info");
    }

    _info = _readerInfo.getInfo(_procReaderId);
  }

  /**
   * Should be called when a ReaderWithBackpressure is not going to be used
   * anymore.
   */
  public void destroy() {
    _readerInfo.free(_procReaderId);
  }

  /**
   * Activate a reader info slot.
   */
  public void activate() {
    _readerPos = _ringBuf.getFreePos();
    _readerInfo.activate(_procReaderId, _readerPos);
  }

  /**
   * Deactivate the allocated reader info slot.
   */
  public void deactivate() {
    _readerInfo.deactivate(_procReaderId);
  }

  /**
   * @return true if the reader is active; false otherwise.
   */
  public boolean isActive() {
    return _info.getIsActive();
  }

  /**
   * @return the reader stream position
   */
  public long pos() {
    return _readerPos;
  }

  /**
   * Read from the reader stream position until the stream end.
   * 
   * @param copyConfirmCb: Invoke Copy for every message. If it returns
   *                       false, message is discarded. If it returns true,
   *                       message processing
   *                       continues. Confirm is called if reader has not expired
   * @return EXPIRED_POSITION if the reader stream position is before the stream
   *         end and the ring buffer has been overwritten. SUCCESS
   *         otherwise.
   */
  public Reader.Result readEx(CopyConfirmCallback copyConfirmCb) {
    assert (isActive());

    Reader.ResultPosPair result = _reader.readEx(_readerPos, _info, copyConfirmCb);
    _readerPos = result.pos;

    assert (result.res != Reader.Result.INVALID_POSITION);

    if (result.res == Reader.Result.EXPIRED_POSITION) {
      _readerInfo.incStatExpiredReaders(1);
    }

    return result.res;
  }
}
