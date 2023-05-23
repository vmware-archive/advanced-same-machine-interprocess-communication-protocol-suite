#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

# Test with message bursts using many readers and writers over several iterations.

set -e

let "WRITERS=${1:-7}"
let "READERS=${2:-16}"
STATARG=${3:-"stat"}

# See config.hpp for list of all options
export TORONI_AGENT_READERS_MAX=$READERS
export TORONI_AGENT_READERS=$READERS
export TORONI_AGENT_WRITERS=$WRITERS

echo "Writers=$WRITERS Readers=$READERS BufSizeKB=$TORONI_AGENT_RINGBUF_SIZE_KB BurstSize=$TORONI_AGENT_MESSAGES_PER_WRITER MessageSizeB=$TORONI_AGENT_MESSAGE_SIZE_BYTES BackpressureSleepMs=$TORONI_AGENT_BACKPRESSURE_SLEEP_MS FLAVOR=$TORONI_AGENT_TEST_FLAVOR Iterations=$TORONI_AGENT_ITERATIONS"

# Init shared memory once for all iterations to get average results
$TORONI_AGENT_DEFAULT init

for (( i=1; i<=$TORONI_AGENT_ITERATIONS; i++ ))
do
   echo "Starting readers"
   for (( a=1; a<=$READERS; a++ ))
   do
      $TORONI_AGENT_READER read &
      rpids[$a]=$!
   done

   # wait readers to start
   $TORONI_AGENT_DEFAULT waitreaders

   echo "Starting writers"
   for (( a=1; a<=$WRITERS; a++ ))
   do
      $TORONI_AGENT_WRITER write &
      wpids[$a]=$!
   done

   echo "Waiting writers to finish"
   for pid in ${wpids[*]}; do
      wait $pid
   done

   echo "Waiting readers to finish"
   for pid in ${rpids[*]}; do
      wait $pid
   done

   $TORONI_AGENT_DEFAULT resetiter
done

# Calculate average stats after all iterations finish
if [ $STATARG == "stat" ]
then
   $TORONI_AGENT_DEFAULT stat
else
   printf "%s" `$TORONI_AGENT_DEFAULT $STATARG` 1>&2
fi

# Exit code 0 if no reader has expired
$TORONI_AGENT_DEFAULT expired
EC=$?

# Cleanup /dev/shm
$TORONI_AGENT_DEFAULT unlink

exit $EC