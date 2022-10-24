/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import java.lang.reflect.Field;

import com.sun.jna.Pointer;
import sun.misc.Unsafe;

import toroni.traits.RobustMutex;

public class ReaderInfoInfo {
  public final long LOCK_OFFSET;
  public final long POSITION_OFFSET;
  public final long IS_ACTIVE_OFFSET;

  public final long INFO_ADDRESS;
  public final long POSITION_ADDRESS;
  public final long IS_ACTIVE_ADDRESS;

  private Pointer _infoPointer;
  public RobustMutex lock;
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
  }

  public ReaderInfoInfo(Pointer infoPointer, RobustMutex _lock) {
    _infoPointer = infoPointer;
    lock = _lock;

    LOCK_OFFSET = 0;
    POSITION_OFFSET = LOCK_OFFSET + lock.size();
    IS_ACTIVE_OFFSET = POSITION_OFFSET + Long.BYTES;

    INFO_ADDRESS = Pointer.nativeValue(_infoPointer);
    POSITION_ADDRESS = INFO_ADDRESS + POSITION_OFFSET;
    IS_ACTIVE_ADDRESS = INFO_ADDRESS + IS_ACTIVE_OFFSET;
  }

  void initialize() {
    setIsActive((byte) 0);
    setPosition(0);
    lock.initialize();
  }

  /**
   * @param mtxSize: size in bytes of a RobustMutex object in memory
   * @return size in bytes of a Info object in memory
   */
  public static long size(long mtxSize) {
    return mtxSize // lock
        + Long.BYTES // position
        // + Byte.BYTES; // isActive, pragma packed(1)
        + Long.BYTES; // isActive
  }

  /**
   * @return the value of {@code position}.
   */
  public long getPosition() {
    return _unsafe.getLongVolatile(null, POSITION_ADDRESS);
  }

  /**
   * Update the value of {@code position} to {@code value}.
   * 
   * @param value
   */
  public void setPosition(long value) {
    _unsafe.putLongVolatile(null, POSITION_ADDRESS, value);
  }

  /**
   * @return the value of {@code isActive}.
   */
  public boolean getIsActive() {
    return (_unsafe.getByteVolatile(null, IS_ACTIVE_ADDRESS) == 1);
  }

  /**
   * Update the value of {@code isActive} to {@code value}.
   * 
   * @param value
   */
  public void setIsActive(byte value) {
    _unsafe.putByteVolatile(null, IS_ACTIVE_ADDRESS, value);
  }

}
