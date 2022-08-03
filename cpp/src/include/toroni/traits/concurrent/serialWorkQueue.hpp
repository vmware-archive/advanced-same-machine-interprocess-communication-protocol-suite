/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_TRAITS_CONCURRENT_SERIALWORKQUEUE_HPP
#define TORONI_TRAITS_CONCURRENT_SERIALWORKQUEUE_HPP

#include "toroni/ref.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace toroni {
namespace traits {
namespace concurrent {
using WorkItem = std::function<void()>;
enum class StopPolicy { IF_EMPTY = false, ALWAYS = true };

/**
 * @brief  A work item queue that executes them serially, i.e. in the same order
 * @note
 * @retval None
 */
class SerialWorkItemQueue
    : public std::enable_shared_from_this<SerialWorkItemQueue> {
public:
  static Ref<SerialWorkItemQueue> Start();
  ~SerialWorkItemQueue();
  void Enqueue(const WorkItem &wi);
  void Stop(StopPolicy stopPolicy = StopPolicy::ALWAYS);

private:
  using WorkItemQueue = std::vector<WorkItem>;

  std::mutex _mtx;
  std::condition_variable _condvar;
  WorkItemQueue _workItemQueue;
  std::atomic<bool> _stop{false};
  std::atomic<StopPolicy> _stopPolicy;
  std::thread _thread;

  SerialWorkItemQueue() = default;
  void Run();
};

inline Ref<SerialWorkItemQueue> SerialWorkItemQueue::Start() {
  Ref<SerialWorkItemQueue> result =
      std::shared_ptr<SerialWorkItemQueue>(new SerialWorkItemQueue());

  result->_thread = std::thread([=]() { result->Run(); });

  return result;
}

inline SerialWorkItemQueue::~SerialWorkItemQueue() { Stop(); }

inline void SerialWorkItemQueue::Enqueue(const WorkItem &wi) {
  std::unique_lock<std::mutex> lock(_mtx);
  _workItemQueue.push_back(wi);
  _condvar.notify_one();
}

/**
 * @brief  Stop the thread processing the queue
 * @note Blocks until the thread finishes.
 * @param  stopPolicy: Whether to stop the thread regardless queue is empty.
 * @retval None
 */
inline void SerialWorkItemQueue::Stop(StopPolicy stopPolicy) {
  if (!_thread.joinable()) {
    return;
  }

  _stop = true;
  _stopPolicy = stopPolicy;
  _condvar.notify_one();
  _thread.join();
}

inline void SerialWorkItemQueue::Run() {
  while (true) {
    WorkItemQueue queueCopy;

    {
      std::unique_lock<std::mutex> lock(_mtx);
      _condvar.wait(lock, [&]() { return !_workItemQueue.empty() || _stop; });

      if (_stop) {
        if (_stopPolicy == StopPolicy::ALWAYS || _workItemQueue.empty()) {
          break;
        }
      }

      queueCopy.swap(_workItemQueue);
    }

    try {
      for (const auto &wi : queueCopy) {
        wi();
      }
    } catch (...) {
    }
  }
}

} // namespace concurrent
} // namespace traits
} // namespace toroni

#endif // TORONI_TRAITS_CONCURRENT_SERIALWORKQUEUE_HPP
