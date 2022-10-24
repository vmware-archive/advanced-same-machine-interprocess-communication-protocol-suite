#
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.18)

project(toroni-java-system-tests VERSION 0.1.0 LANGUAGES NONE)

enable_testing()

configure_file(cpp-java.sh cpp-java.sh COPYONLY)

add_test(NAME InteropTests/Cpp-Java COMMAND ./cpp-java.sh WORKING_DIRECTORY ..)