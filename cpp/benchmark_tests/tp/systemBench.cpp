/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "inMemory.hpp"
#include "rmp/inMemory.hpp"
#include "toroni/tp/asyncWriter.hpp"
#include "toroni/tp/reader.hpp"
#include "toroni/traits/concurrent/mpscMessageQueue.hpp"
#include "toroni/traits/concurrent/serialWorkQueue.hpp"
#include "toroni/traits/posix/multicastUdpNotification.hpp"

#include <benchmark/benchmark.h>

#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <thread>

using namespace std;

namespace toroni {
namespace tp {
namespace bench {

static void TpSystemTransfer(benchmark::State &state) {
  rmp::bench::InMemoryRingBuf ringBuf{state.range(0) * 1024 * 1024};
  InMemoryReaderInfo readerInfo{1};
  traits::concurrent::MPSCMessageQueue<TopicMsgBinaryPtr> msgQueue;
  auto writerWorkQueue = traits::concurrent::SerialWorkItemQueue::Start();
  traits::MulticastUdpNotification un{"226.1.1.1", 3334, "127.0.0.1"};
  Ref<AsyncWriter> writer = AsyncWriter::Create(
      ringBuf.Get(), readerInfo.Get(),
      [&](const auto &p) { return msgQueue.Enqueue(p); },
      [&]() { return msgQueue.Drain(); },
      [&](const auto &p) { writerWorkQueue->Enqueue(p); },
      [&](auto...) {
        this_thread::sleep_for(2ms);
        return false;
      },
      [&]() { un.Send(); });
  const vector<char> msg(state.range(2));
  auto topicMsg =
      writer->CreateMessage("channel", msg.data(), msg.size(), false);
  // reader
  auto readerWorkQueue = traits::concurrent::SerialWorkItemQueue::Start();
  atomic<bool> readerExpired{false};
  Ref<Reader> reader = Reader::Create(
      ringBuf.Get(), readerInfo.Get(),
      [&](const auto &p) { readerWorkQueue->Enqueue(p); },
      [&](const auto &p) { readerWorkQueue->Enqueue(p); },
      [&](ChannelReaderEventType e) { /*1st/last/expired*/
                                      if (e == ALL_CHANNEL_READERS_EXPIRED) {
                                        readerExpired = true;
                                      }
      });
  const int msgCount = state.range(1);
  int rcvCount = 0, totalRcvCount = 0;
  ;

  mutex mtx;
  condition_variable cv;
  atomic<bool> stopReader{false};
  Ref<ChannelReader> channelReadr = reader->CreateChannelReader(
      "channel",
      [&](const auto...) {
        rcvCount++;
        if (rcvCount == msgCount) {
          unique_lock<mutex> lck(mtx);
          cv.notify_one();
        }
      },
      false);
  thread readerThread([&]() {
    while (!stopReader) {
      un.Wait();
      reader->Run();
    }
  });

  for (auto _ : state) {
    rcvCount = 0;

    for (int i = 1; i <= msgCount; i++) {
      writer->Post(topicMsg);
    }

    // wait until reader gets all messages
    unique_lock<mutex> lck(mtx);
    cv.wait(lck, [&]() { return rcvCount == msgCount || readerExpired; });
    totalRcvCount += rcvCount;

    if (readerExpired) {
      state.SkipWithError("Reader expired");
      break;
    }
  }

  stopReader = true;
  un.Send();
  readerThread.join();
  writerWorkQueue->Stop(traits::concurrent::StopPolicy::ALWAYS);
  readerWorkQueue->Stop(traits::concurrent::StopPolicy::ALWAYS);
}
BENCHMARK(TpSystemTransfer)
    // latency 1 msg
    ->Args({1, 1, 10})
    // 100K msg over 4 MB
    ->Args({4, 1000 * 100, 10})
    // 1M msg over 1MB
    ->Args({1, 1000 * 1000, 50});

} // namespace bench
} // namespace tp
} // namespace toroni