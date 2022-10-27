#
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.18)

project(toroni-java-system-tests VERSION 0.1.0 LANGUAGES NONE)

enable_testing()

configure_file(setup-agent.sh setup-agent.sh COPYONLY)
configure_file(bench-table.sh bench-table.sh COPYONLY)
configure_file(stress-no-expire.sh stress-no-expire.sh COPYONLY)
configure_file(robust-writer.sh robust-writer.sh COPYONLY)
configure_file(robust-reader.sh robust-reader.sh COPYONLY)

add_test(NAME SystemTests/StressNoExpire COMMAND ./stress-no-expire.sh WORKING_DIRECTORY ..)
add_test(NAME SystemTests/RobustWriter COMMAND ./robust-writer.sh WORKING_DIRECTORY ..)
add_test(NAME SystemTests/RobustReader COMMAND ./robust-reader.sh WORKING_DIRECTORY ..)