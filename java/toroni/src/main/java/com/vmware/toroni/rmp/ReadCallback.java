/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.rmp;

public interface ReadCallback {
  void messageRecieved(byte[] data, int length);
}
