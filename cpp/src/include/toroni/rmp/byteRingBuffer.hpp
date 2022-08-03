#ifndef TORONI_RMP_BYTERINGBUFFER_HPP
#define TORONI_RMP_BYTERINGBUFFER_HPP

#include "stream.hpp"

namespace toroni {
namespace rmp {

/**
 * @brief  Reliable Message Protocol (RMP) Byte Ring Buffer
 * @note
 */
struct ByteRingBuffer {
  /*
   * Header
   */
  const uint64_t configBufSizeBytes; // Size of the ring buffer in bytes
  traits::RobustProcMutex writerMtx; // Robust mutex for single-writer
  PositionAtomic freePos{0};         // Next free byte position in ring buffer
  struct {
    StatCounter backPressureCount{0};
    StatCounter notificationCount{0};
  } stats;
  const bool initialized; // Whether this structure is completely initialized on
                          // a zero-initialized shared memory
  /*
   *  Ring buffer of configBufSizeBytes
   */

  /**
   * @brief  Returns the size in bytes.
   * @note
   * @param  configBufSizeBytes:
   * @retval Additional bytes.
   */
  static uint32_t Size(uint32_t configBufSizeBytes) {
    return sizeof(ByteRingBuffer) + configBufSizeBytes;
  }

  /**
   * @brief Initialize a ByteRingBuffer with placement new
   * @note See Size
   * @param  bufSizeBytes:
   */
  explicit ByteRingBuffer(uint64_t bufSizeBytes)
      : configBufSizeBytes(bufSizeBytes), initialized(true) {}

  // Ring buffer access
  /**
   * @brief  Ring buffer access
   * @note
   * @param  index: Must be within 0 to configBufSizeBytes
   * @retval Refernce to byte in ringbuffer at index
   */
  char &operator[](int index) {
    return *(reinterpret_cast<char *>(this) + sizeof(*this) + index);
  }
  const char &operator[](int index) const {
    return *(reinterpret_cast<const char *>(this) + sizeof(*this) + index);
  }
};

} // namespace rmp
} // namespace toroni

#endif // TORONI_RMP_BYTERINGBUFFER_HPP
