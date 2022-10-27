/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.system_tests;

import java.lang.reflect.Field;

import com.sun.jna.Pointer;
import sun.misc.Unsafe;

public class AgentStats {

  public static final long LATENCY_NS_SUM_OFFSET = 0;
  public static final long FIRST_LAST_DURATION_NS_SUM_OFFSET = LATENCY_NS_SUM_OFFSET + Long.BYTES;
  public static final long WRITER_DURATION_NS_SUM_OFFSET = FIRST_LAST_DURATION_NS_SUM_OFFSET + Long.BYTES;
  public static final long NOTIFICATION_NS_SUM_OFFSET = WRITER_DURATION_NS_SUM_OFFSET + Long.BYTES;
  public static final long MSG_COUNT_OFFSET = NOTIFICATION_NS_SUM_OFFSET + Long.BYTES;
  public static final long READERS_READY_OFFSET = MSG_COUNT_OFFSET + Long.BYTES;
  public static final long WRITERS_READY_OFFSET = READERS_READY_OFFSET + Long.BYTES;
  public static final long READER_RUNS_OFFSET = WRITERS_READY_OFFSET + Long.BYTES;

  public final long STATS_ADDRESS;
  public final long LATENCY_NS_SUM_ADDRESS;
  public final long FIRST_LAST_DURATION_NS_SUM_ADDRESS;
  public final long WRITER_DURATION_NS_SUM_ADDRESS;
  public final long NOTIFICATION_NS_SUM_ADDRESS;
  public final long MSG_COUNT_ADDRESS;
  public final long READERS_READY_ADDRESS;
  public final long WRITERS_READY_ADDRESS;
  public final long READER_RUNS_ADDRESS;

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

  public AgentStats(Pointer statsPtr) {
    STATS_ADDRESS = Pointer.nativeValue(statsPtr);
    LATENCY_NS_SUM_ADDRESS = STATS_ADDRESS + LATENCY_NS_SUM_OFFSET;
    FIRST_LAST_DURATION_NS_SUM_ADDRESS = STATS_ADDRESS + FIRST_LAST_DURATION_NS_SUM_OFFSET;
    WRITER_DURATION_NS_SUM_ADDRESS = STATS_ADDRESS + WRITER_DURATION_NS_SUM_OFFSET;
    NOTIFICATION_NS_SUM_ADDRESS = STATS_ADDRESS + NOTIFICATION_NS_SUM_OFFSET;
    MSG_COUNT_ADDRESS = STATS_ADDRESS + MSG_COUNT_OFFSET;
    READERS_READY_ADDRESS = STATS_ADDRESS + READERS_READY_OFFSET;
    WRITERS_READY_ADDRESS = STATS_ADDRESS + WRITERS_READY_OFFSET;
    READER_RUNS_ADDRESS = STATS_ADDRESS + READER_RUNS_OFFSET;
  }

  public static long size() {
    return Long.BYTES * 8;
  }

  public void initialize() {
    setLatencyNsSum(0);
    setFirstLastDurationNsSum(0);
    setWriterDurationNsSum(0);
    setNotificationNsSum(0);
    setMsgCount(0);
    setReadersReady(0);
    setWritersReady(0);
    setReaderRuns(0);
  }

  public long getLatencyNsSum() {
    return _unsafe.getLongVolatile(null, LATENCY_NS_SUM_ADDRESS);
  }

  public void setLatencyNsSum(long value) {
    _unsafe.putLongVolatile(null, LATENCY_NS_SUM_ADDRESS, value);
  }

  public void incLatencyNsSum(long value) {
    _unsafe.getAndAddLong(null, LATENCY_NS_SUM_ADDRESS, value);
  }

  public long getFirstLastDurationNsSum() {
    return _unsafe.getLongVolatile(null, FIRST_LAST_DURATION_NS_SUM_ADDRESS);
  }

  public void setFirstLastDurationNsSum(long value) {
    _unsafe.putLongVolatile(null, FIRST_LAST_DURATION_NS_SUM_ADDRESS, value);
  }

  public void incFirstLastDurationNsSum(long value) {
    _unsafe.getAndAddLong(null, FIRST_LAST_DURATION_NS_SUM_ADDRESS, value);
  }

  public long getWriterDurationNsSum() {
    return _unsafe.getLongVolatile(null, WRITER_DURATION_NS_SUM_ADDRESS);
  }

  public void setWriterDurationNsSum(long value) {
    _unsafe.putLongVolatile(null, WRITER_DURATION_NS_SUM_ADDRESS, value);
  }

  public void incWriterDurationNsSum(long value) {
    _unsafe.getAndAddLong(null, WRITER_DURATION_NS_SUM_ADDRESS, value);
  }

  public long getNotificationNsSum() {
    return _unsafe.getLongVolatile(null, NOTIFICATION_NS_SUM_ADDRESS);
  }

  public void setNotificationNsSum(long value) {
    _unsafe.putLongVolatile(null, NOTIFICATION_NS_SUM_ADDRESS, value);
  }

  /*
   * Atomically increase value with new {@code value}
   */
  public void incNotificationNsSum(long value) {
    _unsafe.getAndAddLong(null, NOTIFICATION_NS_SUM_ADDRESS, value);
  }

  public long getMsgCount() {
    return _unsafe.getLongVolatile(null, MSG_COUNT_ADDRESS);
  }

  public void setMsgCount(long value) {
    _unsafe.putLongVolatile(null, MSG_COUNT_ADDRESS, value);
  }

  /*
   * Atomically increase value with new {@code value}
   */
  public void incMsgCount(long value) {
    _unsafe.getAndAddLong(null, MSG_COUNT_ADDRESS, value);
  }

  public long getReadersReady() {
    return _unsafe.getLongVolatile(null, READERS_READY_ADDRESS);
  }

  public void setReadersReady(long value) {
    _unsafe.putLongVolatile(null, READERS_READY_ADDRESS, value);
  }

  /*
   * Atomically increase value with new {@code value}
   */
  public void incReadersReady(long value) {
    _unsafe.getAndAddLong(null, READERS_READY_ADDRESS, value);
  }

  public long getWritersReady() {
    return _unsafe.getLongVolatile(null, WRITERS_READY_ADDRESS);
  }

  public void setWritersReady(long value) {
    _unsafe.putLongVolatile(null, WRITERS_READY_ADDRESS, value);
  }

  /*
   * Atomically increase value with new {@code value}
   */
  public void incWritersReady(long value) {
    _unsafe.getAndAddLong(null, WRITERS_READY_ADDRESS, value);
  }

  public long getReaderRuns() {
    return _unsafe.getLongVolatile(null, READER_RUNS_ADDRESS);
  }

  public void setReaderRuns(long value) {
    _unsafe.putLongVolatile(null, READER_RUNS_ADDRESS, value);
  }

  /*
   * Atomically increase value with new {@code value}
   */
  public void incReaderRuns(long value) {
    _unsafe.getAndAddLong(null, READER_RUNS_ADDRESS, value);
  }
}