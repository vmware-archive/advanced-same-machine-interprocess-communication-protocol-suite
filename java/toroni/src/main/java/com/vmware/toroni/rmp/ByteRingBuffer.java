/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.rmp;

import java.lang.reflect.Field;

import com.sun.jna.Pointer;
import com.vmware.toroni.traits.RobustMutex;

import sun.misc.Unsafe;

public class ByteRingBuffer {

  public final long BUF_SIZE_OFFSET;
  public final long MTX_OFFSET;
  public final long FREE_POS_OFFSET;
  public final long STAT_BACK_PRESSURE_COUNT_OFFSET;
  public final long STAT_NOTIFICATION_COUNT_OFFSET;
  public final long INITIALIZED_OFFSET;
  public final long BUFFER_OFFSET;

  public final long RING_BUF_ADDRESS;
  public final long BUF_SIZE_ADDRESS;
  public final long MTX_ADDRESS;
  public final long FREE_POS_ADDRESS;
  public final long STAT_BACK_PRESSURE_COUNT_ADDRESS;
  public final long STAT_NOTIFICATION_COUNT_ADDRESS;
  public final long INITIALIZED_ADDRESS;
  public final long BUFFER_ADDRESS;

  private final Pointer _ringBufPointer;
  private final long _bufSize;
  private RobustMutex _mtx;
  private static Unsafe _unsafe;
  private static long byteArrayOffset;

  /**
   * Initialize _unsafe with the Unsafe object.
   */
  static {
    try {
      Field f = Unsafe.class.getDeclaredField("theUnsafe");
      f.setAccessible(true);
      _unsafe = (Unsafe) f.get(null);

      // instead of computing it everytime we can precompute it once
      byteArrayOffset = _unsafe.arrayBaseOffset(byte[].class);
    } catch (Exception e) {
      System.out.println("Unsafe couldn't be loaded!");
    }
  }

  public ByteRingBuffer(Pointer ringBufPointer, long bufSizeBytes,
      RobustMutex protoLock) {
    _ringBufPointer = ringBufPointer;
    _bufSize = bufSizeBytes;

    BUF_SIZE_OFFSET = 0;
    MTX_OFFSET = BUF_SIZE_OFFSET + Long.BYTES;
    FREE_POS_OFFSET = MTX_OFFSET + protoLock.size();
    STAT_BACK_PRESSURE_COUNT_OFFSET = FREE_POS_OFFSET + Long.BYTES;
    STAT_NOTIFICATION_COUNT_OFFSET = STAT_BACK_PRESSURE_COUNT_OFFSET + Long.BYTES;
    INITIALIZED_OFFSET = STAT_NOTIFICATION_COUNT_OFFSET + Long.BYTES;
    // BUFFER_OFFSET = INITIALIZED_OFFSET + Byte.BYTES; // pragma packed(1)
    BUFFER_OFFSET = INITIALIZED_OFFSET + Long.BYTES;

    RING_BUF_ADDRESS = Pointer.nativeValue(_ringBufPointer);
    BUF_SIZE_ADDRESS = RING_BUF_ADDRESS + BUF_SIZE_OFFSET;
    MTX_ADDRESS = RING_BUF_ADDRESS + MTX_OFFSET;
    FREE_POS_ADDRESS = RING_BUF_ADDRESS + FREE_POS_OFFSET;
    STAT_BACK_PRESSURE_COUNT_ADDRESS = RING_BUF_ADDRESS + STAT_BACK_PRESSURE_COUNT_OFFSET;
    STAT_NOTIFICATION_COUNT_ADDRESS = RING_BUF_ADDRESS + STAT_NOTIFICATION_COUNT_OFFSET;
    INITIALIZED_ADDRESS = RING_BUF_ADDRESS + INITIALIZED_OFFSET;
    BUFFER_ADDRESS = RING_BUF_ADDRESS + BUFFER_OFFSET;

    _mtx = protoLock.load(new Pointer(MTX_ADDRESS));
  }

  /**
   * Initialize the memory for a non-initialized ByteRingBuffer.
   */
  public void initialize() {
    _unsafe.putLong(BUF_SIZE_ADDRESS, _bufSize);
    _mtx.initialize(new Pointer(MTX_ADDRESS));
    setFreePos(0);
    setStatBackPressureCount(0);
    setStatNotificationCount(0);
    _unsafe.putLong(INITIALIZED_ADDRESS, (byte) 1);
  }

  /**
   * @param bufSize: size of the buffer
   * @param mtxSize: size of the mtx object in memory
   * @return the size in bytes of a ByteRingBuffer object in memory
   */
  public static long size(long bufSize, long mtxSize) {
    return Long.BYTES // bufSize
        + mtxSize // mtx
        + Long.BYTES // freePos
        + Long.BYTES // statBackPressureCount
        + Long.BYTES // statNotificationCount
        // + Byte.BYTES // initialized, pragma packed(1)
        + Long.BYTES // initialized
        + bufSize; // actual buffer
  }

  /**
   * @return the size of the buffer in bytes.
   */
  public long getBufSize() {
    return _unsafe.getLong(BUF_SIZE_ADDRESS);
  }

  /**
   * @return {@code mtx}
   */
  public RobustMutex getMtx() {
    return _mtx;
  }

  /**
   * @return the value of {@code freePos}
   */
  public long getFreePos() {
    return _unsafe.getLongVolatile(null, FREE_POS_ADDRESS);
  }

  /**
   * Updates the value of {@code freePos} to {@code newValue}.
   *
   * @param newValue
   */
  public void setFreePos(long newValue) {
    _unsafe.putLongVolatile(null, FREE_POS_ADDRESS, newValue);
  }

  /*
   * Atomically increase value with new {@code newValue}
   */
  public void incFreePos(long newValue) {
    _unsafe.getAndAddLong(null, FREE_POS_ADDRESS, newValue);
  }

  /**
   * @return the value of {@code statBackPressureCount}
   */
  public long getStatBackPressureCount() {
    return _unsafe.getLongVolatile(null, STAT_BACK_PRESSURE_COUNT_ADDRESS);
  }

  /**
   * Updates the value of {@code statBackPressureCount} to {@code newValue}.
   *
   * @param newValue
   */
  public void setStatBackPressureCount(long newValue) {
    _unsafe.putLongVolatile(null, STAT_BACK_PRESSURE_COUNT_ADDRESS, newValue);
  }

  /*
   * Atomically increase value with new {@code newValue}
   */
  public void incStatBackPressureCount(long newValue) {
    _unsafe.getAndAddLong(null, STAT_BACK_PRESSURE_COUNT_ADDRESS, newValue);
  }

  /**
   * @return the value of {@code statNotificationCount}
   */
  public long getStatNotificationCount() {
    return _unsafe.getLongVolatile(null, STAT_NOTIFICATION_COUNT_ADDRESS);
  }

  /**
   * Updates the value of {@code statNotificationCount} to {@code newValue}.
   *
   * @param newValue
   */
  public void setStatNotificationCount(long newValue) {
    _unsafe.putLongVolatile(null, STAT_NOTIFICATION_COUNT_ADDRESS, newValue);
  }

  /*
   * Atomically increase value with new {@code newValue}
   */
  public void incStatNotificationCount(long newValue) {
    _unsafe.getAndAddLong(null, STAT_NOTIFICATION_COUNT_ADDRESS, newValue);
  }

  /**
   * @return the value of {@code initialized}.
   */
  public boolean getInitialized() {
    return (_unsafe.getByte(INITIALIZED_ADDRESS) == 1);
  }

  /**
   * @param index
   * @return the byte at position {@code index} in the buffer.
   */
  public byte getByte(long index) {
    return _unsafe.getByte(BUFFER_ADDRESS + index);
  }

  /**
   * Writes the byte {@code value} at position {@code index} in the buffer.
   *
   * @param index
   * @param value
   */
  public void setByte(long index, byte value) {
    _unsafe.putByte(BUFFER_ADDRESS + index, value);
  }

  /**
   * Writes the byte array {@code data} starting at position {@code index} in the
   * ring buffer.
   * 
   * @param index
   * @param data
   */
  public void setBytes(long index, byte[] data) {
    _unsafe.copyMemory(data, byteArrayOffset, null, BUFFER_ADDRESS + index, data.length);
  }

  /**
   * Reads (@code length) bytes from the ring buffer starting at {@code index}
   * into {@code data}.
   *
   * @param index
   * @param length
   * @param data
   */
  public void getBytes(long index, long length, byte[] data) {
    _unsafe.copyMemory(null, BUFFER_ADDRESS + index, data, byteArrayOffset,
        length);
  }

  /**
   * @param index
   * @return the int at position {@code index} in the buffer.
   */
  public int getInt(long index) {
    return _unsafe.getInt(BUFFER_ADDRESS + index);
  }

  /**
   * Writes the int {@code value} at position {@code index} in the buffer.
   *
   * @param index
   * @param value
   */
  public void setInt(long index, int value) {
    _unsafe.putInt(BUFFER_ADDRESS + index, value);
  }

  /**
   *
   * @param index
   * @return the long at position {@code index} in the buffer.
   */
  public long getLong(long index) {
    return _unsafe.getLong(BUFFER_ADDRESS + index);
  }

  /**
   * Writes the long {@code value} at position {@code index} in the buffer.
   *
   * @param index
   * @param value
   */
  public void setLong(long index, long value) {
    _unsafe.putLong(BUFFER_ADDRESS + index, value);
  }
}
