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
   * Copies data to local storage.
   * 
   * @param data
   * @param dataLength
   * @return true if successful, false otherwise
   */
  boolean copy(Pointer data, long dataLength);

  /**
   * Invokes a callback once the data has been confirmed.
   */
  void confirm();

}
