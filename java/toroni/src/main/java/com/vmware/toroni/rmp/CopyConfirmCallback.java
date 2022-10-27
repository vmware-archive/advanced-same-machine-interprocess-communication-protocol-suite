/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.rmp;

/**
 * Copies data to local storage and when confirmed invokes a callback.
 */
public interface CopyConfirmCallback {

  /**
   * Copies {@code length} bytes starting from {@index} data to local storage.
   *
   * @param index
   * @param length
   * @return true if successful, false otherwise
   */
  boolean copy(long index, int length);

  /**
   * Invokes a callback once the data has been confirmed.
   */
  void confirm();
}
