/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

public interface BackPressureCallback {
  /**
   * Invoked by a writer if backpressure is detected.
   * 
   * @param bpPos
   * @param freePos
   * @return true if the writer should continue trying taking backpressure into
   *         account; false if the writer should write the message anyway
   */
  boolean writeOrWait(long bpPos, long freePos);
}
