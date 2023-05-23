/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.toroni.traits.posix;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import com.vmware.toroni.traits.SharedMemory;
import com.vmware.toroni.traits.posix.PosixSharedMemoryFactory;

import static com.sun.jna.platform.linux.Fcntl.S_IRUSR;
import static com.sun.jna.platform.linux.Fcntl.S_IWUSR;

class PosixSharedMemoryFactoryTest {
  @Test
  void openNonExistingThrows() {
    Assertions.assertThrows(Exception.class, () -> {
      PosixSharedMemoryFactory.open("OpenNonExistingThrows", 10);
    });
    Assertions.assertThrows(Exception.class, () -> {
      PosixSharedMemoryFactory.openReadOnly("OpenNonExistingThrows", 10);
    });
  }

  @Test
  void createOrOpenReadOnlyThrows() {
    Assertions.assertThrows(Exception.class, () -> {
      PosixSharedMemoryFactory.createOrOpenReadOnly("CreateOrOpenReadOnlyThrows",
          16, S_IRUSR | S_IWUSR);
    });
  }

  @Test
  void createOrOpenUnlinkDoesNotThrow() {
    Assertions.assertDoesNotThrow(() -> {
      SharedMemory shm = PosixSharedMemoryFactory.createOrOpen("CreateOrOpenUnlinkDoesNotThrow",
          16, S_IRUSR | S_IWUSR);
      shm.unlink();
    });
  }

  @Test
  void createOrOpenOpenUnlinkOpen() {
    try {
      SharedMemory shm = PosixSharedMemoryFactory.createOrOpen("CreateOrOpenOpenUnlinkOpen", 16, S_IRUSR | S_IWUSR);
      Assertions.assertDoesNotThrow(() -> {
        PosixSharedMemoryFactory.open("CreateOrOpenOpenUnlinkOpen", 10);
      });

      shm.unlink();

      Assertions.assertThrows(Exception.class, () -> {
        PosixSharedMemoryFactory.open("CreateOrOpenOpenUnlinkOpen", 10);
      });
    } catch (Exception e) {
      assert (false);
    }
  }

  @Test
  void transferData() {
    try {
      SharedMemory shm1 = PosixSharedMemoryFactory.createOrOpen("TransferData",
          16, S_IRUSR | S_IWUSR);
      SharedMemory shm2 = PosixSharedMemoryFactory.createOrOpenReadOnly("TransferData",
          16, S_IRUSR | S_IWUSR);

      shm1.ptr().setInt(0, 20);
      assertEquals(20, shm2.ptr().getInt(0));

      shm1.unlink();
      Assertions.assertThrows(Exception.class, () -> {
        shm2.unlink();
      });
    } catch (Exception e) {
      assert (false);
    }
  }

  @Test
  void writeToReadOnlyDies() {
    /**
     * This test requires us to catch a segmentation fault, which under Java is
     * impossible because the JVM dies whenever a segmentation fault happens.
     * 
     * Possible workaround:
     * https://stackoverflow.com/questions/6344674/how-can-i-write-a-junit-test-for-a-jni-call-that-crashes
     */
  }

  @Test
  void writeBiggerDataDies() {
    try {
      SharedMemory shm = PosixSharedMemoryFactory.createOrOpen("WriteBiggerDataDies",
          16, S_IRUSR | S_IWUSR);

      byte[] buf = new byte[16];
      for (int i = 0; i < 16; i++) {
        buf[i] = 42;
      }

      shm.ptr().write(0, buf, 0, 16);

      shm.unlink();
    } catch (Exception e) {
      assert (false);
    }
  }

  @Test
  void isCreator() {
    try {
      SharedMemory shm1 = PosixSharedMemoryFactory.createOrOpen("IsCreator", 16, S_IRUSR | S_IWUSR);
      SharedMemory shm2 = PosixSharedMemoryFactory.createOrOpen("IsCreator", 16, S_IRUSR | S_IWUSR);
      SharedMemory shm3 = PosixSharedMemoryFactory.createOrOpen("IsCreator", 16, S_IRUSR | S_IWUSR);

      assertEquals(true, shm1.isCreator());
      assertEquals(false, shm2.isCreator());
      assertEquals(false, shm3.isCreator());

      shm1.unlink();
    } catch (Exception e) {
      assert (false);
    }
  }
}
