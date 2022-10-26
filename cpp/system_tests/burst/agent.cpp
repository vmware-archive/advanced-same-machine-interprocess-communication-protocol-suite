/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/traits/concurrent/mpscMessageQueue.hpp"
#include "toroni/traits/concurrent/serialWorkQueue.hpp"
#include "toroni/traits/posix/multicastUdpNotification.hpp"
#include "toroni/traits/posix/robustProcMutex.hpp"
#include "toroni/traits/posix/sharedMemory.hpp"

#include "toroni/tp/asyncWriter.hpp"
#include "toroni/tp/channelReader.hpp"
#include "toroni/tp/reader.hpp"

#include "config.hpp"
#include "log.hpp"

#include <new>

using namespace std;
using namespace toroni;
using namespace toroni::traits;
using namespace toroni::tp;
using namespace toroni::rmp;

const size_t MS_NANOSEC = 1000 * 1000;
const size_t SEC_NANOSEC = 1000 * 1000 * 1000;
inline size_t NowNanoSec() {
  struct timespec monotime;
  clock_gettime(CLOCK_MONOTONIC, &monotime);
  return SEC_NANOSEC * monotime.tv_sec + monotime.tv_nsec;
}

enum EXIT_CODE { EC_SUCCESS, EC_UNKNOWN_ARG, EC_READER_EXPIRED };

struct AgentStats {
  atomic<uint64_t> latencyNsSum;
  atomic<uint64_t> firstLastDurationNsSum;
  atomic<uint64_t> writerDurationNsSum;
  atomic<uint64_t> notificationNsSum;
  atomic<uint64_t> msgCount;
  atomic<uint64_t> readersReady;
  atomic<uint64_t> writersReady;
  atomic<uint64_t> readerRuns;
};

SharedMemory *RingBufShm() {
  static SharedMemory ringBufShm = SharedMemory::CreateOrOpen(
      "toroni-burst-rb", ByteRingBuffer::Size(GetOptRingBufSizeBytes()), 0600);
  return &ringBufShm;
}

ByteRingBuffer *RingBufPtr() {
  return static_cast<ByteRingBuffer *>(RingBufShm()->Ptr());
}

SharedMemory *ReaderInfoShm() {
  static SharedMemory readerInfoShm = SharedMemory::CreateOrOpen(
      "toroni-burst-ri", toroni::tp::ReaderInfo::Size(GetOptMaxReaders()),
      0600);
  return &readerInfoShm;
}

tp::ReaderInfo *ReaderInfoPtr() {
  return static_cast<tp::ReaderInfo *>(ReaderInfoShm()->Ptr());
}

rmp::ReaderInfo *RmpReaderInfoPtr() {
  return static_cast<rmp::ReaderInfo *>(&ReaderInfoPtr()->rmpReaderInfo);
}

SharedMemory *StatsShm() {
  static SharedMemory statsShm =
      SharedMemory::CreateOrOpen("toroni-burst-s", sizeof(AgentStats), 0600);
  return &statsShm;
}

AgentStats *AgentStatsPtr() {
  return static_cast<AgentStats *>(StatsShm()->Ptr());
}

MulticastUdpNotification &UdpNotification() {
  static MulticastUdpNotification un{"224.1.1.1", 3334, "127.0.0.1"};
  return un;
}

void RunInit() {
  new (RingBufShm()->Ptr()) ByteRingBuffer(GetOptRingBufSizeBytes());
  new (ReaderInfoShm()->Ptr()) toroni::tp::ReaderInfo(GetOptMaxReaders());
  memset(StatsShm()->Ptr(), 0, sizeof(AgentStats));
}

template <typename TestPolicy> void RunWriter(TestPolicy &testPolicy) {
  LOG(trace, "writer start");

  size_t notificationNs = 0;
  auto &un = UdpNotification();
  const auto notifyCb = [&]() {
    LOG(trace, "write notify start");
    const size_t startNs = NowNanoSec();
    un.Send();
    notificationNs += (NowNanoSec() - startNs);
    LOG(trace, "writer notify end");
  };

  concurrent::MPSCMessageQueue<TopicMsgBinaryPtr> msgQueue;
  // auto writerWorkQueue = concurrent::SerialWorkItemQueue::Start();
  std::function<void()> wrk;
  Ref<AsyncWriter> writer = AsyncWriter::Create(
      RingBufPtr(), ReaderInfoPtr(),
      [&](const auto &p) { return msgQueue.Enqueue(p); },
      [&]() { return msgQueue.Drain(); },
      [&](const auto &p) { /*writerWorkQueue->Enqueue(p);*/
                           wrk = p;
      },
      [&](auto...) {
        LOG(debug, "writer backpressure");
        testPolicy.OnBackpressure();
        return false;
      },
      notifyCb);

  testPolicy.InitWriter(writer);
  testPolicy.SyncAllWriters();

  LOG(debug, "writer posting start");
  while (testPolicy.PostMore()) {
    testPolicy.Post();
  }
  LOG(debug, "writer posting end");

  // writerWorkQueue->Stop(concurrent::StopPolicy::IF_EMPTY);
  const size_t startNs = NowNanoSec();
  wrk();
  size_t elapsedTimeInNs = NowNanoSec() - startNs;
  AgentStatsPtr()->writerDurationNsSum += elapsedTimeInNs;

  AgentStatsPtr()->notificationNsSum += notificationNs;

  LOG(trace, "writer end");
}

template <typename TestPolicy> void RunReader(TestPolicy &testPolicy) {
  LOG(trace, "reader start");

  auto &un = UdpNotification();
  atomic<bool> readerExpired{false};

  Ref<tp::Reader> reader = tp::Reader::Create(
      RingBufPtr(), ReaderInfoPtr(),
      [&](const auto &p) {
        p(); /*readerWorkQueue->Enqueue(p);*/
      },
      [&](const auto &p) {
        p(); /*readerWorkQueue->Enqueue(p);*/
      },
      [&](ChannelReaderEventType e) { /*1st/last/expired*/
                                      if (e == ALL_CHANNEL_READERS_EXPIRED) {
                                        LOG(error, "reader expired");
                                        readerExpired = true;
                                        exit(EC_READER_EXPIRED);
                                      }
      });

  Ref<ChannelReader> channelReadr = reader->CreateChannelReader(
      "channel", [&](auto... p) { testPolicy.OnChannelRead(p...); }, false);

  // wait to receive all messages
  size_t runCount = 0;
  AgentStatsPtr()->readersReady++;
  LOG(info, "reader started");

  while (testPolicy.ReadMore()) {
    LOG(trace, "reader wait start");
    un.Wait();
    LOG(trace, "reader wait end");

    LOG(trace, "reader run start");
    runCount++;
    reader->Run();
    LOG(trace, "reader run end");
  }

  AgentStatsPtr()->readerRuns += runCount;
  LOG(trace, "reader end");
}

class BaseTest {
public:
  BaseTest()
      : _msgAllWriterCount(GetOptMessagesPerWriter() * GetOptWriters()),
        _msgPerWriterCount(GetOptMessagesPerWriter()),
        _bpSleepMs(GetOptBackpressureSleepMs()) {}
  void OnBackpressure() { this_thread::sleep_for(_bpSleepMs * 1ms); }
  bool PostMore() const { return _postedCount < _msgPerWriterCount; }
  bool ReadMore() const { return _rcvedCount < _msgAllWriterCount; }
  void SyncAllWriters() {
    //  AgentStatsPtr()->writersReady++;
    //  while (AgentStatsPtr()->writersReady < GetOptWriters()) {
    //    this_thread::sleep_for(1ms);
    //  }
    //  LOG(trace, "writer after wait " <<
    //  AgentStatsPtr()->writersReady.load());
  }

protected:
  const size_t _msgAllWriterCount;
  const size_t _msgPerWriterCount;
  const size_t _bpSleepMs;
  Ref<AsyncWriter> _writer;
  size_t _postedCount = 0;
  size_t _rcvedCount = 0;

  static double AvgPerIter(double v) { return v / GetOptIterations(); }

  static void PrintResultExt() {
    auto rb = RingBufPtr();
    auto ri = ReaderInfoPtr();
    auto as = AgentStatsPtr();

    // Avg time per writer in ms to write all messages (including notifications)
    double wtMs = AvgPerIter(1.0 * as->writerDurationNsSum.load() /
                             GetOptWriters() / MS_NANOSEC);
    // Avg time per notification in ms
    double wnMs = 1.0 * as->notificationNsSum.load() /
                  rb->stats.notificationCount.load() / MS_NANOSEC;
    // Avg time per writer for pure writing without notifications
    double wwMs = wtMs - wnMs;

    printf(
        "WT=%.3f_WW=%.3f_WN=%.3f_E=%.3f_N=%.3f_B=%.3f_R=%.3f", wtMs, wwMs, wnMs,
        // Avg number of expired reader events
        AvgPerIter(ri->rmpReaderInfo.stats.expiredReaders.load()),
        // Avg number of sent notifications per writer
        AvgPerIter(1.0 * rb->stats.notificationCount.load() / GetOptWriters()),
        // Avg number of backpressure events
        AvgPerIter(1.0 * rb->stats.backPressureCount.load()),
        // Avg number of reader runs per reader
        AvgPerIter(1.0 * as->readerRuns.load() / GetOptReaders()));
  }
};

class ThroughputTest : public BaseTest {
public:
  void InitWriter(const Ref<AsyncWriter> &w) {
    _writer = w;
    const vector<char> msg(GetOptMessagesSizeBytes());
    _topicMsg =
        _writer->CreateMessage("channel", msg.data(), msg.size(), false);
  }

  void Post() {
    _writer->Post(_topicMsg);
    _postedCount++;
  }

  void OnChannelRead(const void *, size_t) {
    _rcvedCount++;

    if (_rcvedCount == 1) {
      _firstNs = NowNanoSec();
      LOG(trace, "throughput rcv first");
    } else if (_rcvedCount == _msgAllWriterCount) {
      size_t elapsedTimeInNs = NowNanoSec() - _firstNs;
      AgentStatsPtr()->firstLastDurationNsSum += elapsedTimeInNs;
      AgentStatsPtr()->msgCount += _rcvedCount;
      LOG(trace, "throughput rcv last " << elapsedTimeInNs);
    }
  }

  void Result() const {
    printf("%.3f", AvgThroughputKMsgSec());

    if (GetOptExtResult()) {
      printf("_");
      PrintResultExt();
    }
  }

  static double AvgThroughputKMsgSec() {
    double msgCountK = 1.0 * AgentStatsPtr()->msgCount / 1000;
    double durSec =
        (1.0 * AgentStatsPtr()->firstLastDurationNsSum / MS_NANOSEC) / 1000;
    return msgCountK / durSec;
  }

private:
  size_t _firstNs = 0;
  TopicMsgBinaryPtr _topicMsg;
};

class LatencyTest : public BaseTest {
public:
  void InitWriter(const Ref<AsyncWriter> &w) { _writer = w; }

  void Post() {
    size_t now = NowNanoSec();
    auto topicMsg = _writer->CreateMessage("channel", &now, sizeof(now), false);
    _writer->Post(topicMsg);
    _postedCount++;
  }

  void OnChannelRead(const void *data, size_t) {
    _rcvedCount++;
    _latencySumNs += NowNanoSec() - *static_cast<const size_t *>(data);
    LOG(trace, "latenct rcv");
  }

  void Result() const {
    printf("%.3f\n", AvgLatencyMs());

    if (GetOptExtResult()) {
      printf("_");
      PrintResultExt();
    }
  }

  static double AvgLatencyMs() {
    auto bs = AgentStatsPtr();
    return (1.0 * bs->latencyNsSum / MS_NANOSEC) / bs->msgCount;
  }

  ~LatencyTest() {
    AgentStatsPtr()->latencyNsSum += _latencySumNs;
    AgentStatsPtr()->msgCount += _rcvedCount;
  }

private:
  size_t _latencySumNs{0};
};

class RobustWriterTest : public ThroughputTest {
public:
  void OnBackpressure() { exit(EC_SUCCESS); }

  void OnChannelRead(const void *, size_t) {
    // Block the reader so it can never advance
    while (true) {
      this_thread::yield();
    }
  }
};

class RobustReaderTest : public ThroughputTest {
public:
  bool ReadMore() const {
    exit(EC_SUCCESS);
    return true;
  }
};

void RunUnlink() {
  RingBufShm()->Unlink();
  ReaderInfoShm()->Unlink();
  StatsShm()->Unlink();
}

int RunExpired() { return ReaderInfoPtr()->rmpReaderInfo.stats.expiredReaders; }

void RunWaitReadersReady(int expReaders) {
  while (AgentStatsPtr()->readersReady < expReaders)
    ;
}

void RunResetIter() {
  AgentStatsPtr()->writersReady = 0;
  AgentStatsPtr()->readersReady = 0;
}

void RunStat() {
  auto rb = RingBufPtr();
  auto ri = ReaderInfoPtr();
  auto as = AgentStatsPtr();

  printf("Backpressure count:%lu\n", rb->stats.backPressureCount.load());
  printf("Expired reader count:%lu\n",
         ri->rmpReaderInfo.stats.expiredReaders.load());
  printf("Write notifications count:%lu\n", rb->stats.notificationCount.load());
  printf("Freepos:%lu\n", rb->freePos.load());

  uint16_t amin = 0, amax = 0;
  RmpReaderInfoPtr()->GetActiveRange(amin, amax);
  printf("Active reader range [%d,%d)\n", amin, amax);
  printf("Sum latency: %.5f (ms)\n",
         1.0 * as->latencyNsSum.load() / MS_NANOSEC);
  printf("Sum throughput-first-last-duration: %.5f (ms) \n",
         1.0 * as->firstLastDurationNsSum / MS_NANOSEC);
  printf("Avg-latency: %.3f (ms)\n", LatencyTest::AvgLatencyMs());
  printf("Avg-throughput-first-last: %.3f (Kmsg/sec)\n",
         ThroughputTest::AvgThroughputKMsgSec());
  printf("Readers ready: %lu\n", as->readersReady.load());
  printf("Received messages: %lu\n", as->msgCount.load());
}

int RunCmpReceivedMessages(size_t arg2) {
  return AgentStatsPtr()->msgCount == arg2 ? 0 : 1;
}

class OffsetPrinter
{
public:
   OffsetPrinter(const string& name, const void* basePtr) : _basePtr(static_cast<const char*>(basePtr)) {
      printf("%s\n", name.c_str());
   }

   void Offset(const string& field, const void* ptr)
   {
      printf("%s %d\n", field.c_str(), static_cast<const char*>(ptr) - _basePtr);
   }

private:
   const char* _basePtr;
};

void RunLayout()
{
   {
      OffsetPrinter op("RingBuf", RingBufPtr());
      op.Offset("configBufSizeBytes", &(RingBufPtr()->configBufSizeBytes));
      op.Offset("writerMtx", &(RingBufPtr()->writerMtx));
      op.Offset("freePos", &(RingBufPtr()->freePos));
      op.Offset("backPressureCount", &(RingBufPtr()->stats.backPressureCount));
      op.Offset("notificationCount", &(RingBufPtr()->stats.notificationCount));
      op.Offset("initialized", &(RingBufPtr()->initialized));
      op.Offset("index0", &(RingBufPtr()->operator[](0)));
   }

   {
      OffsetPrinter op("ReaderInfo", ReaderInfoPtr());
      op.Offset("readerGen", &(ReaderInfoPtr()->readerGen));
      op.Offset("initialized", &(ReaderInfoPtr()->initialized));
      op.Offset("expiredReaders", &(ReaderInfoPtr()->rmpReaderInfo.stats.expiredReaders));
      op.Offset("configMaxReaders", &(ReaderInfoPtr()->rmpReaderInfo.configMaxReaders));
      op.Offset("readersMinMax", &(ReaderInfoPtr()->rmpReaderInfo.readersMinMax()));
      for (int i=0; i<2; i++) {
         op.Offset("lock", &(ReaderInfoPtr()->rmpReaderInfo.Get(i).lock));
         op.Offset("position", &(ReaderInfoPtr()->rmpReaderInfo.Get(i).position));
         op.Offset("isActive", &(ReaderInfoPtr()->rmpReaderInfo.Get(i).isActive));
      }
   }

   {
      OffsetPrinter op("Stats", AgentStatsPtr());
      op.Offset("latencyNsSum", &(AgentStatsPtr()->latencyNsSum));
      op.Offset("firstLastDurationNsSum", &(AgentStatsPtr()->firstLastDurationNsSum));
      op.Offset("writerDurationNsSum", &(AgentStatsPtr()->writerDurationNsSum));
      op.Offset("notificationNsSum", &(AgentStatsPtr()->notificationNsSum));
      op.Offset("msgCount", &(AgentStatsPtr()->msgCount));
      op.Offset("readersReady", &(AgentStatsPtr()->readersReady));
      op.Offset("writersReady", &(AgentStatsPtr()->writersReady));
      op.Offset("readerRuns", &(AgentStatsPtr()->readerRuns));
   }
}

template <typename TestPolicy>
int Main(int argc, char *argv[], TestPolicy &testPolicy) {
  if (argc == 1) {
    printf("unknown cmd");
    return EC_UNKNOWN_ARG;
  }

  if (string("init") == argv[1]) {
    RunInit();
  } else if (string("write") == argv[1]) {
    RunWriter(testPolicy);
  } else if (string("read") == argv[1]) {
    RunReader(testPolicy);
  } else if (string("stat") == argv[1]) {
    RunStat();
  } else if (string("result") == argv[1]) {
    testPolicy.Result();
  } else if (string("unlink") == argv[1]) {
    RunUnlink();
  } else if (string("expired") == argv[1]) {
    return RunExpired();
  } else if (string("waitreaders") == argv[1]) {
    RunWaitReadersReady(GetOptReaders());
  } else if (string("cmprcved") == argv[1]) {
    return RunCmpReceivedMessages(stoi(argv[2]));
  } else if (string("resetiter") == argv[1]) {
    RunResetIter();
  } else if (string("layout") == argv[1]) {
    RunLayout();
  } else {
    printf("unknown cmd");
    return EC_UNKNOWN_ARG;
  }

  return EC_SUCCESS;
}

int main(int argc, char *argv[]) {
  InitLogger();

  switch (GetOptTestFlavor()) {
  case FIRST_LAST_DURATION: {
    ThroughputTest throughputTest;
    return Main(argc, argv, throughputTest);
    break;
  }
  case LATENCY: {
    LatencyTest latencyTest;
    return Main(argc, argv, latencyTest);
    break;
  }
  case ROBUST_WRITER: {
    RobustWriterTest robustWriterTest;
    return Main(argc, argv, robustWriterTest);
    break;
  }
  case ROBUST_READER: {
    RobustReaderTest robustReaderTest;
    return Main(argc, argv, robustReaderTest);
    break;
  }
  default:
    printf("unknown test flavor");
    exit(EC_UNKNOWN_ARG);
  }
}