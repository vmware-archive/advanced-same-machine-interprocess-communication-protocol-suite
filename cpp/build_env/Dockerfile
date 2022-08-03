# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

FROM rikorose/gcc-cmake:gcc-11

RUN apt-get update -y && apt-get install -y gdb lcov doxygen graphviz clang-format clang-tidy \
python iwyu
RUN apt-get -y install libboost-log-dev
