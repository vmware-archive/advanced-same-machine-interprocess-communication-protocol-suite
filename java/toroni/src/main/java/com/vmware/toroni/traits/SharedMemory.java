/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.traits;

import com.sun.jna.Pointer;

public interface SharedMemory {

  /**
   * Returns whether caller created the shared memory.
   * 
   * @return true if the caller created the shared memory; false otherwise.
   */
  public boolean isCreator();

  /**
   * Pointer to the mapped shared memory.
   * 
   * @return a pointer to the mapped shared memory.
   */
  public Pointer ptr();

  /**
   * Unmaps the shared memory.
   * 
   * @throws Exception
   */
  public void unmap() throws Exception;

  /**
   * Unlinks the shared memory.
   * 
   * @throws Exception
   */
  public void unlink() throws Exception;

}
