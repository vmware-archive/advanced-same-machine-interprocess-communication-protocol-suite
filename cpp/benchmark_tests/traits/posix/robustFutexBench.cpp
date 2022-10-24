/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/posix/robustProcMutex.hpp"

// TODO
#include "../../rmp/inMemory.hpp"

#include <benchmark/benchmark.h>

static void RobustFutexLockUnlock(benchmark::State &state) {
  toroni::rmp::bench::InMemoryRingBuf ringBuf{4 * 1024 * 1024};

  for (auto _ : state) {
   ringBuf.Get()->writerMtx.Lock();
   ringBuf.Get()->writerMtx.Unlock();
  }
}
BENCHMARK(RobustFutexLockUnlock);

static void RobustFutexTryLock(benchmark::State &state) {
  toroni::rmp::bench::InMemoryRingBuf ringBuf{4 * 1024 * 1024};

  for (auto _ : state) {
   ringBuf.Get()->writerMtx.TryLock();
  }
}
BENCHMARK(RobustFutexTryLock);
