#ifndef TORONI_TRAITS_CONCURRENT_MPSCMESSAGEQUEUE_HPP
#define TORONI_TRAITS_CONCURRENT_MPSCMESSAGEQUEUE_HPP

#include <mutex>
#include <vector>

namespace toroni {
namespace traits {
namespace concurrent {

/**
 * @brief  A thread-safe multi-producer, single-consumer queue facilitating the
 * lifecycle of one drainer thread and multiple enqueuer threads.
 * @note
 * @retval None
 */
template <typename T> class MPSCMessageQueue {
public:
  /**
   * @brief  Enqueues a message
   * @note
   * @param  msg:
   * @retval True if a new drainer thread should be started. False otherwise.
   * See Drain
   */
  bool Enqueue(const T &msg) {
    std::unique_lock<std::mutex> lock(_mtx);
    bool result = _startDrainer;
    _startDrainer = false; // do not start a new drainer on next call
    _queue.push_back(msg);
    return result;
  }

  /**
   * @brief Drain all messages from the queue
   * @note
   * @param  &queue:
   * @retval None
   */
  /**
   * @brief Drain all messages from the queue
   * @note
   * @retval The message queue. If empty, the drainer must exit and next call to
   * Enqueue will return true. Otherwise drainer will continue its duty-cycle
   * and next call to Enqueue will return false.
   */
  std::vector<T> Drain() {
    std::vector<T> result;

    std::unique_lock<std::mutex> lock(_mtx);
    result.swap(_queue);

    _startDrainer = result.empty(); // result is empty, drainer will exit, next
                                    // Enqueue should start a new drainer

    return result;
  }

private:
  std::mutex _mtx;
  std::vector<T> _queue;
  bool _startDrainer{true};
};

} // namespace concurrent
} // namespace traits
} // namespace toroni

#endif // TORONI_TRAITS_CONCURRENT_MPSCMESSAGEQUEUE_HPP
