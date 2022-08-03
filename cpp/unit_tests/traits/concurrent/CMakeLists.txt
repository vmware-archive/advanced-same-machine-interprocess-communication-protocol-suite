#
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0
#

target_sources(unit_test
PRIVATE
   mpscMessageQueueTest.cpp
   serialWorkQueueTest.cpp)

target_link_libraries(unit_test PUBLIC rt)