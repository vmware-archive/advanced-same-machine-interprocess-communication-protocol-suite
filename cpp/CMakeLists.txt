cmake_minimum_required(VERSION 3.18)

project(toroni VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

enable_testing()

# enable code coverage
find_package(codecov)

add_subdirectory(src)
add_subdirectory(unit_tests)
add_subdirectory(benchmark_tests)
add_subdirectory(system_tests)
add_subdirectory(doc_gen)

coverage_evaluate()
