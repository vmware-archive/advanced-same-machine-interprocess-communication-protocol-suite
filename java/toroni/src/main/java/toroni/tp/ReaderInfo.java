/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.tp;

import java.lang.reflect.Field;

import com.sun.jna.Pointer;

import sun.misc.Unsafe;

import toroni.traits.RobustMutex;

public class ReaderInfo {
  public static final long READER_GEN_OFFSET;
  public static final long INITIALIZED_OFFSET;
  public static final long RMP_READER_INFO_OFFSET;

  public final long READER_INFO_ADDRESS;
  public final long READER_GEN_ADDRESS;
  public final long INITIALIZED_ADDRESS;
  public final long RMP_READER_INFO_ADDRESS;

  private Pointer _readerInfoPointer;
  private short _maxReaders;
  private static Unsafe _unsafe;

  /**
   * Initialize _unsafe with the Unsafe object.
   */
  static {
    try {
      Field f = Unsafe.class.getDeclaredField("theUnsafe");
      f.setAccessible(true);
      _unsafe = (Unsafe) f.get(null);
    } catch (Exception e) {
      System.out.println("Unsafe couldn't be loaded!");
    }

    READER_GEN_OFFSET = 0;
    INITIALIZED_OFFSET = READER_GEN_OFFSET + Long.BYTES;
    // RMP_READER_INFO_OFFSET = INITIALIZED_OFFSET + Byte.BYTES; // pragma pack(1)
    RMP_READER_INFO_OFFSET = INITIALIZED_OFFSET + Long.BYTES;
  }

  public toroni.rmp.ReaderInfo rmpReaderInfo;

  public ReaderInfo(Pointer readerInfoPointer, short maxReaders, RobustMutex[] locks) {
    _readerInfoPointer = readerInfoPointer;
    _maxReaders = maxReaders;

    READER_INFO_ADDRESS = Pointer.nativeValue(_readerInfoPointer);
    READER_GEN_ADDRESS = READER_INFO_ADDRESS + READER_GEN_OFFSET;
    INITIALIZED_ADDRESS = READER_INFO_ADDRESS + INITIALIZED_OFFSET;
    RMP_READER_INFO_ADDRESS = READER_INFO_ADDRESS + RMP_READER_INFO_OFFSET;

    rmpReaderInfo = new toroni.rmp.ReaderInfo(new Pointer(RMP_READER_INFO_ADDRESS), _maxReaders, locks);
  }

  /**
   * @param maxReaders: maximum allowed number of active readers
   * @param mtxSize:    size in bytes of a RobustMutex object in memory
   * @return the size in bytes of a ReaderInfo object in memory
   */
  public static long size(short maxReaders, long mtxSize) {
    return Long.BYTES // readerGen
        // + Byte.BYTES // initialized, pragma pack(1)
        + Long.BYTES // initialized
        + toroni.rmp.ReaderInfo.size(maxReaders, mtxSize); // rmpReaderInfo
  }

  /**
   * Initialize the memory for a non-initialized ReaderInfo.
   */
  public void initialize() {
    _unsafe.putLong(READER_GEN_ADDRESS, 0);
    _unsafe.putByte(INITIALIZED_ADDRESS, (byte) 1);
    rmpReaderInfo.initialize();
  }

  /**
   * @return the value of {@code initialized}.
   */
  public boolean getInitialized() {
    return (_unsafe.getByte(INITIALIZED_ADDRESS) == (byte) 1);
  }

  /**
   * @return the value of {@code readerGen}.
   */
  public long getReaderGen() {
    return _unsafe.getLongVolatile(null, READER_GEN_ADDRESS);
  }

  /**
   * Update the value of {@code readerGen} to {@code value}.
   * 
   * @param value
   */
  public void setReaderGen(long value) {
    _unsafe.putLongVolatile(null, READER_GEN_ADDRESS, value);
  }

}
