#
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0
#

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(
  unit_test
  main.cpp
)
target_link_libraries(unit_test
PRIVATE
  gtest_main
  gmock_main
  toroni
)
target_include_directories(unit_test PRIVATE .)

add_subdirectory(rmp)
add_subdirectory(tp)
add_subdirectory(traits)

# Exclude gtest from coverage
list(APPEND LCOV_REMOVE_PATTERNS "'${CMAKE_BINARY_DIR}/*'" "'${CMAKE_CURRENT_SOURCE_DIR}/*'")
add_coverage(unit_test)

include(GoogleTest)
gtest_discover_tests(unit_test)