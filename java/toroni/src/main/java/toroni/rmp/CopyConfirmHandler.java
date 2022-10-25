/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

public class CopyConfirmHandler implements CopyConfirmCallback {
  private ByteRingBuffer _ringBuf;
  private ReadCallback _readCb;
  private byte[] _data;

  public CopyConfirmHandler(ByteRingBuffer ringBuf, ReadCallback readCb) {
    _ringBuf = ringBuf;
    _readCb = readCb;
  }

  public boolean copy(long index, long dataLength) {
    _data = new byte[(int) dataLength];
    _ringBuf.getBytes(index, dataLength, _data);
    return true;
  }

  public void confirm() {
    _readCb.messageRecieved(_data);
  }
}
