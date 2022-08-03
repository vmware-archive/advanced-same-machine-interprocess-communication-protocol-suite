#include "toroni/traits/posix/robustProcMutex.hpp"

#include "toroni/rmp/byteRingBuffer.hpp"
#include "toroni/rmp/reader.hpp"
#include "toroni/rmp/readerInfo.hpp"
#include "toroni/rmp/readerWithBackpressure.hpp"
#include "toroni/rmp/stream.hpp"
#include "toroni/rmp/writer.hpp"

#include "toroni/ref.hpp"
#include "toroni/tp/asyncWriter.hpp"
#include "toroni/tp/channelReader.hpp"
#include "toroni/tp/detail/topicMsgBinaryDeserializer.hpp"
#include "toroni/tp/detail/topicMsgBinarySerializer.hpp"
#include "toroni/tp/reader.hpp"
#include "toroni/tp/readerInfo.hpp"

#include <benchmark/benchmark.h>

BENCHMARK_MAIN();
