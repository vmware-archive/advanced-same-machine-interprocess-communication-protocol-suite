/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.traits;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.platform.unix.LibCAPI;
import com.sun.jna.platform.unix.LibCUtil;

import static com.sun.jna.platform.linux.Fcntl.O_CREAT;
import static com.sun.jna.platform.linux.Fcntl.O_RDONLY;
import static com.sun.jna.platform.linux.Fcntl.O_RDWR;
import static com.sun.jna.platform.linux.Fcntl.O_EXCL;
import static com.sun.jna.platform.linux.Mman.MAP_SHARED;
import static com.sun.jna.platform.linux.Mman.MAP_FAILED;
import static com.sun.jna.platform.linux.Mman.PROT_READ;
import static com.sun.jna.platform.linux.Mman.PROT_WRITE;
import static com.sun.jna.platform.linux.ErrNo.EEXIST;

public class PosixSharedMemoryFactory {
  private static LibRT LIBRT = LibRT.INSTANCE;
  private static LibC LIBC = LibC.INSTANCE;

  /**
   * Opens or creates a shared memory in read-only mode.
   * 
   * @param name:   name of the shared memory.
   * @param length: size in bytes of the shared memory.
   * @param mode:   specifies the file mode bits to be
   *                applied when a new file is created.
   * @return a SharedMemory object for the corresponding shared memory.
   * @throws Exception
   */
  public static SharedMemory createOrOpenReadOnly(String name, int length, int mode) throws Exception {
    return createOrOpen(name, length, mode, O_RDONLY, PROT_READ);
  }

  /**
   * Opens or creates a shared memory in read-write mode.
   * 
   * @param name:   name of the shared memory.
   * @param length: size in bytes of the shared memory.
   * @param mode:   specifies the file mode bits to be
   *                applied when a new file is created.
   * @return a SharedMemory object for the corresponding shared memory.
   * @throws Exception
   */
  public static SharedMemory createOrOpen(String name, long length, int mode) throws Exception {
    return createOrOpen(name, length, mode, O_RDWR, PROT_WRITE);
  }

  /**
   * Opens shared memory in read-only mode. Throws if does not exist.
   * 
   * @param name:   name of the shared memory.
   * @param length: size in bytes of the shared memory.
   * @return a SharedMemory object for the corresponding shared memory.
   * @throws Exception
   */
  public static SharedMemory openReadOnly(String name, long length) throws Exception {
    return open(name, length, O_RDONLY, PROT_READ);
  }

  /**
   * Opens shared memory in read-write mode. Throws if does not exist.
   * 
   * @param name:   name of the shared memory.
   * @param length: size in bytes of the shared memory.
   * @return a SharedMemory object for the corresponding shared memory.
   * @throws Exception
   */
  public static SharedMemory open(String name, long length) throws Exception {
    return open(name, length, O_RDWR, PROT_WRITE);
  }

  /**
   * Opens or creates a shared memory with the given permissions.
   * 
   * @param name:   name of the shared memory.
   * @param length: size in bytes of the shared memory.
   * @param mode:   specifies the file mode bits to be
   *                applied when a new file is created.
   * @param oflag:  a bit mask created by ORing together exactly one of O_RDONLY
   *                or O_RDWR and any of the other flags O_CREAT, O_EXCL, or
   *                O_TRUNC.
   * @param prot:   describes the desired memory protection of the
   *                mapping
   * @return a SharedMemory object for the corresponding shared memory.
   * @throws Exception
   */
  private static SharedMemory createOrOpen(String name, long length, int mode, int oflag, int prot) throws Exception {
    boolean isCreator = false;
    Pointer ptr = Pointer.NULL;
    int tr = 0;

    int fd = LIBRT.shm_open(name, O_CREAT | O_EXCL | oflag, mode);
    try {
      if (fd == -1) {
        if (Native.getLastError() == EEXIST) {
          return open(name, length, oflag, prot);
        } else {
          throw new Exception("shm_open");
        }
      } else {
        isCreator = true;
        try {
          tr = LibCUtil.ftruncate(fd, length);
        } catch (Exception e) {
          tr = -1; // (WORKAROUND) ftruncate throws instead of returning -1
        }
      }

      if (tr == -1) {
        LIBRT.shm_unlink(name);
        throw new Exception("ftruncate");
      }

      ptr = LibCUtil.mmap(null, length, prot, MAP_SHARED, fd, 0);
      if (ptr == MAP_FAILED) {
        throw new Exception("mmap");
      }

      return new PosixSharedMemory(name, ptr, length, isCreator);
    } finally {
      if (fd != -1) {
        LIBC.close(fd);
      }
    }
  }

  /**
   * Opens shared memory with the given permissions.
   * 
   * @param name:   name of the shared memory.
   * @param length: size in bytes of the shared memory.
   * @param oflag:  a bit mask created by ORing together exactly one of O_RDONLY
   *                or O_RDWR and any of the other flags O_CREAT, O_EXCL, or
   *                O_TRUNC.
   * @param prot:   describes the desired memory protection of the
   *                mapping
   * @return a SharedMemory object for the corresponding shared memory.
   * @throws Exception
   */
  private static SharedMemory open(String name, long length, int oflag, int prot) throws Exception {
    boolean isCreator = false;
    Pointer ptr = Pointer.NULL;

    int fd = LIBRT.shm_open(name, oflag, 0);

    try {
      if (fd == -1) {
        throw new Exception("shm_open");
      }

      ptr = LibCUtil.mmap(null, length, prot, MAP_SHARED, fd, 0);
      if (ptr == MAP_FAILED) {
        throw new Exception("mmap");
      }

      return new PosixSharedMemory(name, ptr, length, isCreator);
    } finally {
      if (fd != -1) {
        LIBC.close(fd);
      }
    }
  }

  /**
   * Posix implementation of the SharedMemory interface
   */
  private static class PosixSharedMemory implements SharedMemory {

    private String _name;
    private Pointer _ptr;
    private long _length;
    private boolean _isCreator;

    public PosixSharedMemory(String name, Pointer ptr, long length, boolean isCreator) {
      _name = name;
      _ptr = ptr;
      _length = length;
      _isCreator = isCreator;
    }

    /**
     * Returns whether caller created the shared memory.
     * 
     * @return true if the caller created the shared memory; false otherwise.
     */
    @Override
    public boolean isCreator() {
      return _isCreator;
    }

    /**
     * Pointer to the mapped shared memory.
     * 
     * @return a pointer to the mapped shared memory.
     */
    @Override
    public Pointer ptr() {
      return _ptr;
    }

    /**
     * Unmaps the shared memory.
     * 
     * @throws Exception
     */
    @Override
    public void unmap() throws Exception {
      if (LIBC.munmap(_ptr, new LibCAPI.size_t(_length)) != 0) {
        throw new Exception("munmap");
      }
    }

    /**
     * Unlinks the shared memory.
     * 
     * @throws Exception
     */
    @Override
    public void unlink() throws Exception {
      if (LIBRT.shm_unlink(_name) == -1) {
        throw new Exception("shm_unlink");
      }
    }

  }

  /**
   * Interface for LibRT. Required for JNA.
   */
  private interface LibRT extends Library {
    LibRT INSTANCE = Native.load("rt", LibRT.class);

    int shm_open(String name, int oflag, int mode);

    int shm_unlink(String name);
  }

  /**
   * Interface for LibC. Required for JNA.
   */
  private interface LibC extends LibCAPI, Library {
    LibC INSTANCE = Native.load("c", LibC.class);
  }
}
