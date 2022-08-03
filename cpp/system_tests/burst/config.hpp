#ifndef TORONI_CPP_SYSTEM_TESTS_BURST_CONFIG_HPP
#define TORONI_CPP_SYSTEM_TESTS_BURST_CONFIG_HPP

#include <cstdlib>
#include <string>

std::string GetOptString(const char *optName, const char *defaultValue) {
  if (const char *e = std::getenv(optName)) {
    return e;
  } else {
    return defaultValue;
  }
}

int GetOptInt(const char *optName, int defaultValue) {
  if (const char *e = std::getenv(optName)) {
    return std::stoi(e);
  } else {
    return defaultValue;
  }
}

size_t GetOptRingBufSizeBytes() {
  static size_t val =
      GetOptInt("TORONI_AGENT_RINGBUF_SIZE_KB", 4 * 1024) * 1024;
  return val;
}

size_t GetOptMaxReaders() {
  static size_t val = GetOptInt("TORONI_AGENT_READERS_MAX", 1);
  return val;
}

size_t GetOptReaders() {
  static size_t val = GetOptInt("TORONI_AGENT_READERS", 1);
  return val;
}

size_t GetOptWriters() {
  static size_t val = GetOptInt("TORONI_AGENT_WRITERS", 1);
  return val;
}

size_t GetOptMessagesPerWriter() {
  static size_t val = GetOptInt("TORONI_AGENT_MESSAGES_PER_WRITER", 1);
  return val;
}

size_t GetOptMessagesSizeBytes() {
  static size_t val = GetOptInt("TORONI_AGENT_MESSAGE_SIZE_BYTES", 8);
  return val;
}

size_t GetOptBackpressureSleepMs() {
  static size_t val = GetOptInt("TORONI_AGENT_BACKPRESSURE_SLEEP_MS", 5);
  return val;
}

size_t GetOptExtResult() {
  static size_t val = GetOptInt("TORONI_AGENT_EXTRESULT", 0);
  return val;
}

size_t GetOptIterations() {
  static size_t val = GetOptInt("TORONI_AGENT_ITERATIONS", 1);
  return val;
}

enum TestFlavor {
  UNKNOWN,
  FIRST_LAST_DURATION,
  LATENCY,
  ROBUST_WRITER,
  ROBUST_READER
};
TestFlavor GetOptTestFlavor() {
  static std::string val =
      GetOptString("TORONI_AGENT_TEST_FLAVOR", "FIRST_LAST_DURATION");

  if (val == "FIRST_LAST_DURATION") {
    return FIRST_LAST_DURATION;
  } else if (val == "LATENCY") {
    return LATENCY;
  } else if (val == "ROBUST_WRITER") {
    return ROBUST_WRITER;
  } else if (val == "ROBUST_READER") {
    return ROBUST_READER;
  } else {
    assert(false);
  }

  return UNKNOWN;
}
#endif // TORONI_CPP_SYSTEM_TESTS_BURST_CONFIG_HPP
