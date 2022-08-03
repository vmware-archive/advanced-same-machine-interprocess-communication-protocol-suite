/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/posix/robustProcMutex.hpp"

#include "inMemory.hpp"
#include "toroni/rmp/copyConfirmHandler.hpp"

#include <benchmark/benchmark.h>

namespace toroni {
namespace rmp {
namespace bench {

static const vector<char> msg(64);

static void RmpWriteWithActiveReaders(benchmark::State &state) {
  InMemoryRingBuf ringBuf{4 * 1024 * 1024};
  InMemoryReaderInfo readerInfo{256};
  Writer writer{ringBuf.Get(), readerInfo.Get()};

  for (int i = 0; i < state.range(0); i++) {
    readerInfo.Get()->Activate(i, 0);
  }

  for (auto _ : state) {
    writer.WriteEx(msg.data(), msg.size(), [](auto... param) { return false; });
  }
}
BENCHMARK(RmpWriteWithActiveReaders)->DenseRange(0, 256, 64);

static void RmpWriteWithBufSizeMB(benchmark::State &state) {
  InMemoryRingBuf ringBuf{state.range(0) * 1024 * 1024};
  InMemoryReaderInfo readerInfo{256};
  Writer writer{ringBuf.Get(), readerInfo.Get()};

  for (auto _ : state) {
    writer.WriteEx(msg.data(), msg.size(), [](auto... param) { return false; });
  }
}
BENCHMARK(RmpWriteWithBufSizeMB)->Arg(1)->Arg(4)->Arg(16)->Arg(256);

static void RmpSweepingWriteReadWithBufSizeMB(benchmark::State &state) {
  InMemoryRingBuf ringBuf{state.range(0) * 1024 * 1024};
  InMemoryReaderInfo readerInfo{256};
  Writer writer{ringBuf.Get(), readerInfo.Get()};
  Reader reader{ringBuf.Get()};

  rmp::CopyConfirmHandler cch([&](auto... param) {});
  rmp::PositionAtomic pos1{0}, pos2;

  for (auto _ : state) {
    writer.WriteEx(msg.data(), msg.size(), [](auto... param) { return false; });
    reader.ReadEx(pos1, pos2, cch);
  }
}
BENCHMARK(RmpSweepingWriteReadWithBufSizeMB)->Arg(1)->Arg(4)->Arg(16)->Arg(256);

static void RmpRead(benchmark::State &state) {
  InMemoryRingBuf ringBuf{4 * 1024 * 1024};
  InMemoryReaderInfo readerInfo{256};
  Writer writer{ringBuf.Get(), readerInfo.Get()};
  Reader reader{ringBuf.Get()};
  writer.WriteEx(msg.data(), msg.size(), {});

  rmp::CopyConfirmHandler cch([&](auto... param) {});

  for (auto _ : state) {
    rmp::PositionAtomic pos1{0}, pos2;
    reader.ReadEx(pos1, pos2, cch);
  }
}
BENCHMARK(RmpRead);

} // namespace bench
} // namespace rmp
} // namespace toroni