/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
 
package com.vmware.toroni.system_tests;

public class RobustWriterTest extends ThroughputTest {

  public void onBackpressure() {
    System.exit(Agent.EXIT_CODE.EC_SUCCESS.ordinal());
  }

  public void onChannelReader(byte[] data) {
    while (true) {
      Thread.yield();
    }
  }

}
