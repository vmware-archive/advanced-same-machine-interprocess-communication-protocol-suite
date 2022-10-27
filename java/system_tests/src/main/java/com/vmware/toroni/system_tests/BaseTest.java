/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.system_tests;

import com.vmware.toroni.rmp.ByteRingBuffer;
import com.vmware.toroni.tp.AsyncWriter;

public abstract class BaseTest implements TestPolicy {

  protected final long _msgAllWriterCount;
  protected final long _msgPerWriterCount;
  protected final long _bpSleepMs;
  protected AsyncWriter _writer;
  protected long _postedCount = 0;
  protected long _recievedCount = 0;

  public BaseTest() {
    _msgAllWriterCount = Config.getOptMessagesPerWriter() * Config.getOptWriters();
    _msgPerWriterCount = Config.getOptMessagesPerWriter();
    _bpSleepMs = Config.getOptBackpressureSleepMs();
  }

  public void onBackpressure() {
    try {
      Thread.sleep(_bpSleepMs);
    } catch (Exception e) {
      throw new Error(e);
    }
  }

  public boolean postMore() {
    return _postedCount < _msgPerWriterCount;
  }

  public boolean readMore() {
    return _recievedCount < _msgAllWriterCount;
  }

  public void syncAllWriters() {

  }

  protected static double avgPerIter(double v) {
    return v / Config.getOptIterations();
  }

  protected static void printResultExt() {
    ByteRingBuffer rb = Agent.ringBuf;
    com.vmware.toroni.tp.ReaderInfo ri = Agent.readerInfo;
    AgentStats as = Agent.agentStats;

    double wtMs = avgPerIter((double) as.getWriterDurationNsSum()
        / Config.getOptWriters() / Agent.MS_NANOSEC);

    double wnMs = (double) as.getNotificationNsSum() / rb.getStatNotificationCount() / Agent.MS_NANOSEC;

    double wwMs = wtMs - wnMs;

    System.out.format("WT=%.3f_WW=%.3f_WN=%.3f_E=%.3f_N=%.3f_B=%.3f_R=%.3f%n",
        wtMs, wwMs, wnMs,
        avgPerIter(ri.rmpReaderInfo.getStatExpiredReaders()),
        avgPerIter((double) rb.getStatNotificationCount() / Config.getOptWriters()),
        avgPerIter((double) rb.getStatBackPressureCount()),
        avgPerIter((double) as.getReaderRuns() / Config.getOptReaders()));
  }

}
