/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.tp.detail;

public class Util {

  /**
   * Converts a long to a byte array.
   * 
   * @param x
   * @return tha value of {@code x} as a byte array (least significant byte is
   *         first)
   */
  public static byte[] longToByteArray(long x) {
    return new byte[] {
        (byte) ((x >> 0) & 0xff),
        (byte) ((x >> 8) & 0xff),
        (byte) ((x >> 16) & 0xff),
        (byte) ((x >> 24) & 0xff),
        (byte) ((x >> 32) & 0xff),
        (byte) ((x >> 40) & 0xff),
        (byte) ((x >> 48) & 0xff),
        (byte) ((x >> 56) & 0xff)
    };
  }

  /**
   * Extracts a long value out of an array of bytes.
   * 
   * This method reads 8 bytes starting from the given {@code from} position
   * and converts them into a single {@code long} value.
   * 
   * @param data
   * @param from the position to read 8 bytes from
   * @return the value of data as a long
   */
  public static long readLongValue(byte[] data, int from) {
    assert (from + 7 <= data.length);

    long value = 0;
    for (int i = 7; i >= 0; i--) {
      value = (value << 8) + (data[from + i] & 255);
    }

    return value;
  }
}
