/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import com.sun.jna.Pointer;

public class CopyConfirmHandler implements CopyConfirmCallback {
  private ReadCallback _readCb;
  private byte[] _data;

  public CopyConfirmHandler(ReadCallback readCb) {
    _readCb = readCb;
  }

  public boolean copy(Pointer data, long dataLength) {
    _data = data.getByteArray(0, (int) dataLength);
    return true;
  }

  public void confirm() {
    _readCb.messageRecieved(_data);
  }
}
