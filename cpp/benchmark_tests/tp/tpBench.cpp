/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "inMemory.hpp"
#include "rmp/inMemory.hpp"

#include <benchmark/benchmark.h>

namespace toroni {
namespace tp {
namespace bench {

struct Tp : public benchmark::Fixture {
  const vector<char> msg{64};

  rmp::bench::InMemoryRingBuf ringBuf{4 * 1024 * 1024};
  InMemoryReaderInfo readerInfo{1};
  vector<TopicMsgBinaryPtr> msgQueue;
  Ref<AsyncWriter> writer{AsyncWriter::Create(
      ringBuf.Get(), readerInfo.Get(),
      [&](const auto &msg) {
        msgQueue.push_back(msg);
        return true;
      },
      [&]() {
        vector<TopicMsgBinaryPtr> tmp;
        tmp.swap(msgQueue);
        return tmp;
      },
      [](const auto &workFn) { workFn(); }, {}, []() {})};

  TopicMsgBinaryPtr tpMsg{
      writer->CreateMessage("/channel/foo/bar", msg.data(), msg.size(), true)};
};

BENCHMARK_F(Tp, CreateMessage)(benchmark::State &state) {
  for (auto _ : state) {
    writer->CreateMessage("/channel/foo/bar", msg.data(), msg.size(), true);
  }
}

BENCHMARK_F(Tp, PostProcWriter)(benchmark::State &state) {
  for (auto _ : state) {
    writer->Post(tpMsg);
  }
}

BENCHMARK_F(Tp, SweepPostRead)(benchmark::State &state) {
  auto workFn = [](const auto &readFn) { readFn(); };
  Ref<Reader> reader = Reader::Create(ringBuf.Get(), readerInfo.Get(), workFn,
                                      workFn, [](auto...) {});
  reader->CreateChannelReader(
      "/channel/foo/bar", [](auto... p) {}, false);

  for (auto _ : state) {
    writer->Post(tpMsg);
    reader->Run();
  }
}

} // namespace bench
} // namespace tp
} // namespace toroni