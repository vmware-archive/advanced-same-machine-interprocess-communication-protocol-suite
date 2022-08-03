#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

# Run a matrix of tests. Type of test is set by
#export TORONI_AGENT_TEST_FLAVOR=FIRST_LAST_DURATION
#export TORONI_AGENT_TEST_FLAVOR=LATENCY
#export TORONI_AGENT_EXTRESULT=1

set -e

for r in  1 4 8 16 # {1..16..1}
do
   printf "Readers=%d" $r
   for w in {1..7}
   do
        printf ", "
      ./bench-cell.sh $w $r result  > /dev/null
   done
   printf "\n"
done