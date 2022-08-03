include(FetchContent)

SET(BENCHMARK_ENABLE_GTEST_TESTS false)
SET(BENCHMARK_ENABLE_TESTING false)

FetchContent_Declare(
  google-benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG 1ee7bee6c54bb2b9c5cd055e759cb1acec4bc4ec
)

FetchContent_MakeAvailable(google-benchmark)

add_executable(benchmark_tests
   traits/posix/multicastUdpNotificationBench.cpp
   rmp/rmpBench.cpp
   rmp/notificationBench.cpp
   tp/tpBench.cpp
   tp/systemBench.cpp
   main.cpp)
target_link_libraries(benchmark_tests toroni benchmark::benchmark)
target_include_directories(benchmark_tests PRIVATE .)
