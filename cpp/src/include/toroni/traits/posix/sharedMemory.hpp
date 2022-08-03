/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_TRAITS_POSIX_SHAREDMEMORY_HPP
#define TORONI_TRAITS_POSIX_SHAREDMEMORY_HPP

#include "toroni/exception.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

#include <functional>
#include <string>

namespace toroni {
namespace traits {
/**
 * @brief  Wraper of posix shared memory
 * @note Unmapped at process exit. Zero-initialized. Does not Unmap or Unlink
 * * upon destruction.
 * @retval None
 */
class SharedMemory {
public:
  static SharedMemory CreateOrOpenReadOnly(const char *name, size_t length,
                                           mode_t mode);
  static SharedMemory CreateOrOpen(const char *name, size_t length,
                                   mode_t mode);
  static SharedMemory OpenReadOnly(const char *name, size_t length);
  static SharedMemory Open(const char *name, size_t length);

  bool IsCreator() const;
  void *Ptr();
  void Unmap();
  void Unlink();

private:
  std::string _name;
  void *const _ptr;
  size_t _length;
  const bool _isCreator;

  SharedMemory(const char *name, void *ptr, size_t length, bool isCreator);
  static SharedMemory Open(const char *name, size_t length, int oflag,
                           int prot);
  static SharedMemory CreateOrOpen(const char *name, size_t length, mode_t mode,
                                   int oflag, int prot);
};

/**
 * @brief  Atomically creates or opens shared memory in read-only mode.
 * @note
 * @param  *name:
 * @param  length:
 * @param  mode:
 * @retval The shared memory.
 */
inline SharedMemory SharedMemory::CreateOrOpenReadOnly(const char *name,
                                                       size_t length,
                                                       mode_t mode) {
  return CreateOrOpen(name, length, mode, O_RDONLY, PROT_READ);
}

/**
 * @brief  Atomically creates or opens shared memory in read-write mode.
 * @note
 * @param  *name:
 * @param  length:
 * @param  mode:
 * @retval
 */
inline SharedMemory SharedMemory::CreateOrOpen(const char *name, size_t length,
                                               mode_t mode) {
  return CreateOrOpen(name, length, mode, O_RDWR, PROT_WRITE);
}

/**
 * @brief  Opens shared memory in read-only mode. Throws if does not exist.
 * @note
 * @param  *name:
 * @param  length:
 * @retval
 */
inline SharedMemory SharedMemory::OpenReadOnly(const char *name,
                                               size_t length) {
  return Open(name, length, O_RDONLY, PROT_READ);
}

/**
 * @brief  Opens shared memory in read-write mode. Throws if does not exist.
 * @note
 * @param  *name:
 * @param  length:
 * @retval
 */
inline SharedMemory SharedMemory::Open(const char *name, size_t length) {
  return Open(name, length, O_RDWR, PROT_WRITE);
}

/**
 * @brief  Returns whether caller created the shared memory.
 * @note
 * @retval True if this caller created the shared memory. False otherwise.
 */
inline bool SharedMemory::IsCreator() const { return _isCreator; }

/**
 * @brief  Pointer to the mapped shared memory
 * @note
 * @retval None
 */
inline void *SharedMemory::Ptr() { return _ptr; }

/**
 * @brief  Unmaps the shared memory
 * @note
 * @retval None
 */
inline void SharedMemory::Unmap() {
  if (munmap(_ptr, _length)) {
    throw exception("munmap");
  }
}

/**
 * @brief  Unlinks the shared memory
 * @note
 * @retval None
 */
inline void SharedMemory::Unlink() {
  if (shm_unlink(_name.c_str()) == -1) {
    throw exception("shm_unlink");
  }
}

struct Finally {
  ~Finally() { fun(); }
  std::function<void()> fun;
};

inline SharedMemory::SharedMemory(const char *name, void *ptr, size_t length,
                                  bool isCreator)
    : _name(name), _ptr(ptr), _length(length), _isCreator(isCreator) {}

inline SharedMemory SharedMemory::Open(const char *name, size_t length,
                                       int oflag, int prot) {
  bool isCreator = false;
  void *ptr = nullptr;

  int fd = shm_open(name, oflag, 0);

  Finally closefd{[&]() {
    if (fd != -1) {
      close(fd);
    }
  }};

  if (fd == -1) {
    throw exception("shm_open");
  }

  ptr = mmap(nullptr, length, prot, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    throw exception("mmap");
  }

  return SharedMemory{name, ptr, length, isCreator};
}

inline SharedMemory SharedMemory::CreateOrOpen(const char *name, size_t length,
                                               mode_t mode, int oflag,
                                               int prot) {

  bool isCreator = false;
  void *ptr = nullptr;
  int tr = 0;

  int fd = shm_open(name, O_CREAT | O_EXCL | oflag, mode);
  Finally closefd{[&]() {
    if (fd != -1) {
      close(fd);
    }
  }};

  if (fd == -1) {
    if (errno == EEXIST) {
      return Open(name, length, oflag, prot);
    } else {
      throw exception("shm_open");
    }
  } else {
    isCreator = true;
    tr = ftruncate(fd, length); // the extended part reads as null bytes
  }

  if (tr == -1) {
    shm_unlink(name);
    throw exception("ftruncate");
  }

  ptr = mmap(nullptr, length, prot, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    throw exception("mmap");
  }

  return SharedMemory{name, ptr, length, isCreator};
}

} // namespace traits
} // namespace toroni

#endif // TORONI_TRAITS_POSIX_SHAREDMEMORY_HPP
