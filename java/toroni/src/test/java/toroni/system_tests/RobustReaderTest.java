/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
 
package toroni.system_tests;

public class RobustReaderTest extends ThroughputTest {

  public boolean readMore() {
    System.exit(Agent.EXIT_CODE.EC_SUCCESS.ordinal());
    return true;
  }

}
