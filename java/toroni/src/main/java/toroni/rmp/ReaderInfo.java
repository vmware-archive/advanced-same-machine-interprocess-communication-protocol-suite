/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import java.lang.reflect.Field;

import com.sun.jna.Pointer;
import sun.misc.Unsafe;

import toroni.traits.RobustMutex;

public class ReaderInfo {

  public static final long STAT_EXPIRED_READERS_OFFSET;
  public static final long INITIALIZED_OFFSET;
  public static final long MAX_READERS_OFFSET;
  public static final long READERS_MIN_MAX_OFFSET;
  public static final long FIRST_INFO_OFFSET;

  public final long READER_INFO_ADDRESS;
  public final long STAT_EXPIRED_READERS_ADDRESS;
  public final long INITIALIZED_ADDRESS;
  public final long MAX_READERS_ADDRESS;
  public final long READERS_MIN_MAX_ADDRESS;
  public final long FIRST_INFO_ADDRESS;

  public static final int INVALID_READER_ID = -1;

  private Pointer _readerInfoPointer;
  private short _maxReaders;
  private ReaderInfoInfo[] _infos;
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

    STAT_EXPIRED_READERS_OFFSET = 0;
    INITIALIZED_OFFSET = STAT_EXPIRED_READERS_OFFSET + Long.BYTES;
    // MAX_READERS_OFFSET = INITIALIZED_OFFSET + Byte.BYTES; // pragma pack(1)
    MAX_READERS_OFFSET = INITIALIZED_OFFSET + Short.BYTES;
    READERS_MIN_MAX_OFFSET = MAX_READERS_OFFSET + Short.BYTES;
    FIRST_INFO_OFFSET = READERS_MIN_MAX_OFFSET + Integer.BYTES;
  }

  public ReaderInfo(Pointer readerInfoPointer, short maxReaders, RobustMutex protoLock) {
    _readerInfoPointer = readerInfoPointer;
    _maxReaders = maxReaders;
    _infos = new ReaderInfoInfo[maxReaders];

    READER_INFO_ADDRESS = Pointer.nativeValue(_readerInfoPointer);
    STAT_EXPIRED_READERS_ADDRESS = READER_INFO_ADDRESS + STAT_EXPIRED_READERS_OFFSET;
    INITIALIZED_ADDRESS = READER_INFO_ADDRESS + INITIALIZED_OFFSET;
    MAX_READERS_ADDRESS = READER_INFO_ADDRESS + MAX_READERS_OFFSET;
    READERS_MIN_MAX_ADDRESS = READER_INFO_ADDRESS + READERS_MIN_MAX_OFFSET;
    FIRST_INFO_ADDRESS = READER_INFO_ADDRESS + FIRST_INFO_OFFSET;

    for (int i = 0; i < maxReaders; i++) {
      _infos[i] = new ReaderInfoInfo(new Pointer(FIRST_INFO_ADDRESS + i * ReaderInfoInfo.size(protoLock.size())),
          protoLock);
    }
  }

  /**
   * Initialize the memory for a non-initialized ReaderInfo.
   */
  public void initialize() {
    _unsafe.putLong(STAT_EXPIRED_READERS_ADDRESS, 0);
    _unsafe.putShort(MAX_READERS_ADDRESS, _maxReaders);
    _unsafe.putInt(READERS_MIN_MAX_ADDRESS, 0);

    for (int i = 0; i < _maxReaders; i++) {
      _infos[i].initialize();
    }

    _unsafe.putByte(INITIALIZED_ADDRESS, (byte) 1);
  }

  /**
   * @param maxReaders: maximum allowed number of active readers
   * @param mtxSize:    size in bytes of a RobustMutex object in memory
   * @return the size in bytes of a ReaderInfo object in memory
   */
  public static long size(short maxReaders, long mtxSize) {
    return Long.BYTES // expiredReaders
        // + Byte.BYTES // initialized, pragma packed(1)
        + Short.BYTES // initialized
        + Short.BYTES // maxReaders
        + Integer.BYTES // readersMinMax
        + maxReaders * ReaderInfoInfo.size(mtxSize); // infos
  }

  /**
   * Get reader info slot.
   * 
   * @param readerId: id of the slot
   * @return object of type Info corresponding to the given id
   */
  public ReaderInfoInfo getInfo(int readerId) {
    assert (readerId >= 0 && readerId < _maxReaders);
    return _infos[readerId];
  }

  /**
   * Allocates info slot and returns its id.
   * 
   * @return id if success; INVALID_READER_ID if fail
   */
  public int alloc() {
    for (int i = 0; i < _maxReaders; i++) {
      if (_infos[i].lock.tryLock()) {
        deactivate(i);
        return i;
      }
    }

    return INVALID_READER_ID;
  }

  /**
   * Frees a ReaderInfo slot. It must be freed on the same thread where
   * Alloc was called.
   * 
   * @param readerId
   */
  public void free(int readerId) {
    if (readerId == INVALID_READER_ID) {
      return;
    }

    deactivate(readerId);
    _infos[readerId].lock.unlock();
  }

  /**
   * Activate an allocated reader info slot so its stream position is
   * taken into consideration for creating backpressure on writers.
   * 
   * @param readerId
   * @param pos
   */
  public void activate(int readerId, long pos) {
    if (readerId == INVALID_READER_ID) {
      return;
    }

    _infos[readerId].setIsActive((byte) 1);
    _infos[readerId].setPosition(pos);
    updateActiveRange();
  }

  /**
   * Deactivate an activated reader info slot so its stream position is n
   * longer taken into consideration for creating backpressure on writers.
   * 
   * @param readerId
   */
  public void deactivate(int readerId) {
    if (readerId == INVALID_READER_ID) {
      return;
    }

    _infos[readerId].setIsActive((byte) 0);
    updateActiveRange();
  }

  /**
   * Get the range of active slots. Interval is [min, max) (some slots in the
   * interval might be inactive).
   * 
   * @return an array with 2 elements:
   *         1) min
   *         2) max
   */
  public short[] getActiveRange() {
    int readersMinMax = getReadersMinMax();
    short[] activeRange = { (short) (readersMinMax >> 16), (short) readersMinMax };
    return activeRange;
  }

  /**
   * Update the range of active slots.
   */
  public void updateActiveRange() {
    int old = 0;
    int packed = 0;

    do {
      old = getReadersMinMax();

      short min = _maxReaders, max = 0;
      for (short i = 0; i < _maxReaders; i++) {
        if (_infos[i].getIsActive()) {
          if (i < min) {
            min = i;
          }
          if (i > max) {
            max = i;
          }
        }
      }

      packed = (min << 16) | (max + 1);
    } while (!_unsafe.compareAndSwapInt(null, READERS_MIN_MAX_ADDRESS, old, packed));
  }

  /**
   * @return the value of {@code expiredReaders}.
   */
  public long getStatExpiredReaders() {
    return _unsafe.getLongVolatile(null, STAT_EXPIRED_READERS_ADDRESS);
  }

  /**
   * Update the value of {@code expiredReaders} to {@code value}.
   * 
   * @param value
   */
  public void setStatExpiredReaders(long value) {
    _unsafe.putLongVolatile(null, STAT_EXPIRED_READERS_ADDRESS, value);
  }

  /*
   * Atomically increase value with new {@code value}
   */
  public void incStatExpiredReaders(long value) {
    _unsafe.getAndAddLong(null, STAT_EXPIRED_READERS_ADDRESS, value);
  }

  /**
   * @return the value of {@code initialized}.
   */
  public boolean getInitialized() {
    return (_unsafe.getByte(INITIALIZED_ADDRESS) == (byte) 1);
  }

  /**
   * @return the value of {@code readersMinMax}.
   */
  public int getReadersMinMax() {
    return _unsafe.getIntVolatile(null, READERS_MIN_MAX_ADDRESS);
  }

  /**
   * Update the value of {@code readersMinMax} to {@code value}.
   * 
   * @param value
   */
  public void setReadersMinMax(int value) {
    _unsafe.putIntVolatile(null, READERS_MIN_MAX_ADDRESS, value);
  }

}
