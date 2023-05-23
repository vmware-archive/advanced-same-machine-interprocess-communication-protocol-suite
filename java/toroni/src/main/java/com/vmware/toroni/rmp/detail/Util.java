/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.rmp.detail;

import com.vmware.toroni.rmp.ByteRingBuffer;

public class Util {
  public static boolean greaterThan(long a, long b) {
    return (b - a < 0);
  }

  public static boolean greaterThanOrEqualTo(long a, long b) {
    return (b - a <= 0);
  }

  public static boolean expired(long pos, long freePos, long bufSize) {
    return greaterThanOrEqualTo(freePos, pos + bufSize);
  }

  public static boolean isZeroOrPowerOfTwo(long x) {
    return ((x & (x - 1)) == 0);
  }

  public static long indexMask(long bufSize) {
    assert (isZeroOrPowerOfTwo(bufSize));
    return bufSize - 1;
  }

  public static long bufIndex(long indexMask, long pos) {
    return indexMask & pos;
  }

  public static MessageHeader readMsgHeader(ByteRingBuffer ringBuf, long bufIndex) {
    MessageHeader msgHeader = new MessageHeader();
    msgHeader.type = ringBuf.getByte(bufIndex + MessageHeader.TYPE_OFFSET);
    msgHeader.length = ringBuf.getInt(bufIndex + MessageHeader.LENGTH_OFFSET);
    return msgHeader;
  }
}
