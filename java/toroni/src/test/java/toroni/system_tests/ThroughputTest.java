/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.system_tests;

import java.nio.ByteBuffer;

import toroni.tp.AsyncWriter;

public class ThroughputTest extends BaseTest {

  private long _firstNs;
  byte[] _topicMsg;

  public void initWriter(AsyncWriter w) {
    _writer = w;
    String msg = "a";
    msg = msg.repeat((int) Config.getOptMessagesSizeBytes());

    try {
      _topicMsg = _writer.createMessage("channel", msg.getBytes(), false);
    } catch (Exception e) {
      throw new Error(e);
    }
  }

  public void post() {
    _writer.post(_topicMsg);
    _postedCount++;
  }

  public void onChannelReader(ByteBuffer data) {
    _recievedCount++;

    if (_recievedCount == 1) {
      _firstNs = System.nanoTime();
      Agent.LOGGER.info("throughput rcv first");
    } else if (_recievedCount == _msgAllWriterCount) {
      long elapsedTimeInNs = System.nanoTime() - _firstNs;
      AgentStats as = Agent.agentStats;
      as.incFirstLastDurationNsSum(elapsedTimeInNs);
      as.incMsgCount(_recievedCount);
      Agent.LOGGER.info("throuput rcv last " + elapsedTimeInNs);
    }
  }

  public void result() {
    System.out.print(String.format("%.3f", avgThroughputKMsgSec()));

    if (Config.getOptExtResult() == 1) {
      System.out.print("_");
      printResultExt();
    }
  }

  public static double avgThroughputKMsgSec() {
    AgentStats as = Agent.agentStats;

    double msgCountK = (double) as.getMsgCount() / 1000;
    double durSec = ((double) as.getFirstLastDurationNsSum() / Agent.MS_NANOSEC) / 1000;

    return msgCountK / durSec;
  }

}
