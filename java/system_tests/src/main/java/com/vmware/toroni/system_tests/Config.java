/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.system_tests;

public class Config {

  public static String getOptString(String optName, String defaultValue) {
    try {
      String e = System.getenv(optName);
      if (e == null) {
        return defaultValue;
      }
      return e;
    } catch (Exception e) {
      return defaultValue;
    }
  }

  public static long getOptLong(String optName, long defaultValue) {
    try {
      String e = System.getenv(optName);
      if (e == null) {
        return defaultValue;
      }
      return Long.parseLong(e);
    } catch (Exception e) {
      return defaultValue;
    }
  }

  public static int getOptInt(String optName, int defaultValue) {
    try {
      String e = System.getenv(optName);
      return Integer.parseInt(e);
    } catch (Exception e) {
      return defaultValue;
    }
  }

  public static long getOptRingBufSize() {
    return getOptLong("TORONI_AGENT_RINGBUF_SIZE_KB", 4 * 1024) * 1024;
  }

  public static long getOptMaxReaders() {
    return getOptLong("TORONI_AGENT_READERS", 1);
  }

  public static long getOptReaders() {
    return getOptLong("TORONI_AGENT_READERS", 1);
  }

  public static long getOptWriters() {
    return getOptLong("TORONI_AGENT_WRITERS", 1);
  }

  public static long getOptMessagesPerWriter() {
    return getOptLong("TORONI_AGENT_MESSAGES_PER_WRITER", 1);
  }

  public static long getOptMessagesSizeBytes() {
    return getOptLong("TORONI_AGENT_MESSAGE_SIZE_BYTES", 8);
  }

  public static long getOptBackpressureSleepMs() {
    return getOptLong("TORONI_AGENT_BACKPRESSURE_SLEEP_MS", 5);
  }

  public static long getOptExtResult() {
    return getOptLong("TORONI_AGENT_EXTRESULT", 0);
  }

  public static long getOptIterations() {
    return getOptLong("TORONI_AGENT_ITERATIONS", 1);
  }

  public static enum TestFlavour {
    UNKNOWN, FIRST_LAST_DURATION, LATENCY, ROBUST_WRITER, ROBUST_READER
  }

  public static TestFlavour getOptTestFavour() {
    String val = getOptString("TORONI_AGENT_TEST_FLAVOR", "FIRST_LAST_DURATION");

    if (val.equals("FIRST_LAST_DURATION")) {
      return TestFlavour.FIRST_LAST_DURATION;
    } else if (val.equals("LATENCY")) {
      return TestFlavour.LATENCY;
    } else if (val.equals("ROBUST_WRITER")) {
      return TestFlavour.ROBUST_WRITER;
    } else if (val.equals("ROBUST_READER")) {
      return TestFlavour.ROBUST_READER;
    } else {
      assert (false);
    }

    return TestFlavour.UNKNOWN;
  }

}