/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.toroni.traits;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import com.vmware.toroni.traits.MulticastUdpNotification;

class MulticastUdpNotificationTest {

  private MulticastUdpNotification un;

  @BeforeEach
  void init() {
    try {
      un = new MulticastUdpNotification("226.1.1.1", (short) 3334, "127.0.0.1");
    } catch (Exception e) {
      throw new Error(e);
    }
  }

  @Test
  void matchNotifications() {
    Assertions.assertDoesNotThrow(() -> {
      un.sendNotification();
    });
    Assertions.assertDoesNotThrow(() -> {
      un.sendNotification();
    });
    Assertions.assertDoesNotThrow(() -> {
      un.waitForNotification();
    });
    Assertions.assertDoesNotThrow(() -> {
      un.waitForNotification();
    });
  }

  @Test
  void testWaitBlock() {
    for (int i = 0; i < 5; i++) {
      ExecutorService executor = Executors.newSingleThreadExecutor();
      Future<Void> future = executor.submit(new Callable<Void>() {

        @Override
        public Void call() throws Exception {
          un.waitForNotification();
          return null;
        }

      });

      Assertions.assertThrows(Exception.class, () -> {
        future.get(100, TimeUnit.MILLISECONDS);
      });

      un.sendNotification();

      Assertions.assertDoesNotThrow(() -> {
        future.get(100, TimeUnit.MILLISECONDS);
      });
    }
  }

}
