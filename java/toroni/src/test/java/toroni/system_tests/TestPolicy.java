/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
 
package toroni.system_tests;

import java.nio.ByteBuffer;

import toroni.tp.AsyncWriter;

public interface TestPolicy {
  void onBackpressure();

  void initWriter(AsyncWriter w);

  void syncAllWriters();

  boolean postMore();

  void post();

  void onChannelReader(ByteBuffer data);

  boolean readMore();

  void result();
}
