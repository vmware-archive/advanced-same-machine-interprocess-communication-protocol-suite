# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

FROM rikorose/gcc-cmake:gcc-11 AS dev-env

RUN apt-get update -y && apt-get install -y gdb lcov doxygen graphviz clang-format clang-tidy \
python iwyu
RUN apt-get -y install libboost-log-dev

FROM dev-env AS builder-debug
WORKDIR /toroni/cpp
COPY . .
RUN cmake -S . -B ./build-debug -DCMAKE_BUILD_TYPE=Debug
RUN cmake --build ./build-debug --config Debug

FROM builder-debug AS unit-test
RUN /toroni/cpp/build-debug/unit_tests/unit_test

FROM rikorose/gcc-cmake:gcc-11 AS builder-release
# Not using dev-env which has boost installed which links burst agent to boost
WORKDIR /toroni/cpp
COPY . .
RUN cmake -S . -B ./build-release -DCMAKE_BUILD_TYPE=Release
RUN cmake --build ./build-release --config Release

FROM builder-release AS system-test
WORKDIR /toroni/cpp/build-release/system_tests
RUN make test

FROM ubuntu:22.04 AS system-test-burst
WORKDIR /burst
COPY --from=builder-release /toroni/cpp/build-release/system_tests/burst /burst