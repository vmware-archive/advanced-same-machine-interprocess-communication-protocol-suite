#include "toroni/traits/posix/multicastUdpNotification.hpp"

#include <benchmark/benchmark.h>

struct MulticastUdpNotification : public benchmark::Fixture {
  toroni::traits::MulticastUdpNotification un{"226.1.1.1", 3334, "127.0.0.1"};
};

BENCHMARK_F(MulticastUdpNotification, Send)(benchmark::State &state) {
  for (auto _ : state) {
    un.Send();
  }
}

BENCHMARK_F(MulticastUdpNotification, SendWait)(benchmark::State &state) {
  for (auto _ : state) {
    un.Send();
    un.Wait();
  }
}

BENCHMARK_F(MulticastUdpNotification, Peek)(benchmark::State &state) {
  for (auto _ : state) {
    un.Peek();
  }
}
