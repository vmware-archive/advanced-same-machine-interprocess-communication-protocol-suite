#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

# Run a matrix of tests.

# Type of test is set by one of:
#export TORONI_AGENT_TEST_FLAVOR=FIRST_LAST_DURATION
#export TORONI_AGENT_MESSAGES_PER_WRITER=20000
#export TORONI_AGENT_TEST_FLAVOR=LATENCY
#export TORONI_AGENT_MESSAGES_PER_WRITER=10
#export TORONI_AGENT_EXTRESULT=1

export TORONI_AGENT_TEST_FLAVOR=${TORONI_AGENT_TEST_FLAVOR:-"FIRST_LAST_DURATION"}
export TORONI_AGENT_MESSAGES_PER_WRITER=${TORONI_AGENT_MESSAGES_PER_WRITER:-20000}

export TORONI_AGENT_RINGBUF_SIZE_KB=${TORONI_AGENT_RINGBUF_SIZE_KB:-4096}
export TORONI_AGENT_BACKPRESSURE_SLEEP_MS=${TORONI_AGENT_BACKPRESSURE_SLEEP_MS:-5}
export TORONI_AGENT_ITERATIONS=${TORONI_AGENT_ITERATIONS:-100}
export TORONI_AGENT_MESSAGE_SIZE_BYTES=${TORONI_AGENT_MESSAGE_SIZE_BYTES:-8}

set -e

source setup-agent.sh

for r in  1 4 8 16 # {1..16..1}
do
   printf "Readers=%d" $r
   for w in {1..7}
   do
        printf ", "
      ./bench-cell.sh $w $r result  > /dev/null

      if [ -n "$TORONI_BENCH11" ]; then exit 0; fi
   done
   printf "\n"
done