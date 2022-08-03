#include "toroni/traits/posix/robustProcMutex.hpp"

#include "inMemory.hpp"
#include "toroni/rmp/copyConfirmHandler.hpp"
#include "toroni/traits/posix/multicastUdpNotification.hpp"

#include <benchmark/benchmark.h>

#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <thread>
using namespace std;

namespace toroni {
namespace rmp {
namespace bench {

static const vector<char> msg(10);

static void RmpTransferWithNotificationOnBp(benchmark::State &state) {
  InMemoryRingBuf ringBuf{state.range(0) * 1024 * 1024};
  InMemoryReaderInfo readerInfo{1};
  Writer writer{ringBuf.Get(), readerInfo.Get()};
  ReaderWithBackpressure readerBP(ringBuf.Get(), readerInfo.Get());
  readerBP.Activate();
  traits::MulticastUdpNotification un{"226.1.1.1", 3334, "127.0.0.1"};

  mutex mtx;
  condition_variable cv;

  const int msgCount = 1000 * state.range(1);
  int rcvCount;
  atomic<bool> stopReader{false};
  atomic<bool> readerExpired{false};

  const vector<char> msg(state.range(2));

  thread readerThread([&] {
    while (!stopReader) {
      un.Wait();

      rmp::CopyConfirmHandler cch([&](auto... param) {
        rcvCount++;

        // notify writer all messages are received
        if (rcvCount == msgCount) {
          unique_lock<mutex> lck(mtx);
          cv.notify_one();
        }
      });

      if (readerBP.ReadEx(cch) == rmp::Reader::EXPIRED_POSITION) {
        unique_lock<mutex> lck(mtx);
        cv.notify_one();
        readerExpired = true;
        break;
      }
    }
  });

  auto backpressureFn = [&](auto... p) {
    // notify on backpressure
    un.Send();

    // give reader some time to catch up
    this_thread::sleep_for(state.range(3) * 1ms);

    return false;
  };

  for (auto _ : state) {
    rcvCount = 0;

    for (int i = 1; i <= msgCount; i++) {
      writer.WriteEx(msg.data(), msg.size(), backpressureFn);
    }

    // notify after last message
    un.Send();

    // wait until reader gets all messages
    unique_lock<mutex> lck(mtx);
    cv.wait(lck, [&]() { return rcvCount == msgCount || readerExpired; });

    if (readerExpired) {
      state.SkipWithError("Reader expired");
      break;
    }
  }

  stopReader = true;
  un.Send();

  readerThread.join();
}
BENCHMARK(RmpTransferWithNotificationOnBp)
    /*backpressure hit*/
    ->Args({1, 100, 10, 1})
    ->Args({1, 100, 10, 2})
    ->Args({1, 100, 10, 6})
    /*backpressure not hit*/
    ->Args({4, 100, 10, 2});
;

} // namespace bench
} // namespace rmp
} // namespace toroni