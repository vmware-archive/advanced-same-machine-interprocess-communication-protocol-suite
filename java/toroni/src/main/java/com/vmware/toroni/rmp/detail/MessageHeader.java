/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.rmp.detail;

public class MessageHeader {
  public static final byte MESSAGE = 0;
  public static final byte PADDING = 1;

  public static final int TYPE_OFFSET = 0;
  public static final int LENGTH_OFFSET = TYPE_OFFSET + Byte.BYTES;

  public byte type;
  public long length;

  public boolean valid() {
    return ((type == MESSAGE || type == PADDING) && length != 0);
  }

  public static int size() {
    return Byte.BYTES // type
        + Integer.BYTES; // length
  }
}
