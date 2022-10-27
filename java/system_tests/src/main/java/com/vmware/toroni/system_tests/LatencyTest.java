/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.system_tests;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.vmware.toroni.tp.AsyncWriter;

public class LatencyTest extends BaseTest {

  private long _latencySumNs = 0;

  public void initWriter(AsyncWriter w) {
    _writer = w;
  }

  public void post() {
    long now = System.nanoTime();

    try {
      ByteBuffer dataBB = ByteBuffer.allocate(Long.BYTES);
      dataBB.order(ByteOrder.LITTLE_ENDIAN);
      dataBB.putLong(now);

      byte[] topicMsg = _writer.createMessage("channel",
          dataBB.array(), false);

      _writer.post(topicMsg);
      _postedCount++;
    } catch (Exception e) {
      throw new Error(e);
    }
  }

  public void onChannelReader(ByteBuffer dataBB) {
    _recievedCount++;
    dataBB.order(ByteOrder.LITTLE_ENDIAN);
    _latencySumNs += System.nanoTime() - dataBB.getLong();
    Agent.LOGGER.info("PROCESS_ID [ " + ProcessHandle.current().pid() + " ] " + "latency rcv" + _recievedCount);
  }

  public void result() {
    System.out.println(avgLatencyMs());

    if (Config.getOptExtResult() == 1) {
      System.out.print("_");
      printResultExt();
    }
  }

  public static double avgLatencyMs() {
    AgentStats as = Agent.agentStats;
    return ((double) as.getLatencyNsSum() / Agent.MS_NANOSEC) / as.getMsgCount();
  }

  public void destroy() {
    AgentStats as = Agent.agentStats;

    as.incLatencyNsSum(_latencySumNs);
    as.incMsgCount(_recievedCount);
  }

}
