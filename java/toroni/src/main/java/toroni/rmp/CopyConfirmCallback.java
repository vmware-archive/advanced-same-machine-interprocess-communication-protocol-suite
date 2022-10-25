/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.rmp;

import com.sun.jna.Pointer;

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
  boolean copy(long index, long length);

  /**
   * Invokes a callback once the data has been confirmed.
   */
  void confirm();

}
