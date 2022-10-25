/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

public class CopyConfirmHandler implements CopyConfirmCallback {
  private ByteRingBuffer _ringBuf;
  private ReadCallback _readCb;
  private byte[] _data = new byte[4096];
  private int _length;

  public CopyConfirmHandler(ByteRingBuffer ringBuf, ReadCallback readCb) {
    _ringBuf = ringBuf;
    _readCb = readCb;
  }

  public boolean copy(long index, int length) {
    if (length > _data.length) {
      _data = new byte[length];
    }

    _ringBuf.getBytes(index, length, _data);
    _length = length;

    return true;
  }

  public void confirm() {
    _readCb.messageRecieved(_data, _length);
  }
}
