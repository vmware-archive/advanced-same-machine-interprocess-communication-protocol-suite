#!/bin/bash

# Test with message bursts using many readers and writers over several iterations.

set -e

let "WRITERS=${1:-7}"
let "READERS=${2:-16}"
STATARG=${3:-"stat"}
let "ITERATIONS=${4:-100}"

# See config.hpp for list of all options
export TORONI_AGENT_READERS_MAX=$READERS
export TORONI_AGENT_READERS=$READERS
export TORONI_AGENT_WRITERS=$WRITERS
export  TORONI_AGENT_ITERATIONS=$ITERATIONS
#export TORONI_AGENT_RINGBUF_SIZE_KB=128
#export TORONI_AGENT_BACKPRESSURE_SLEEP_MS=5

if [ "$TORONI_AGENT_TEST_FLAVOR" == "LATENCY" ]
then
   export TORONI_AGENT_MESSAGES_PER_WRITER=10
else
   export TORONI_AGENT_MESSAGES_PER_WRITER=20000
fi

echo "Writers=$WRITERS Readers=$READERS Burst Size=$TORONI_AGENT_MESSAGES_PER_WRITER"

# Init shared memory once for all iterations to get average results
./agent init

for (( i=1; i<=$ITERATIONS; i++ ))
do
   echo "Starting readers"
   for (( a=1; a<=$READERS; a++ ))
   do
      ./agent read &
      rpids[$a]=$!
   done

   # wait readers to start
   ./agent waitreaders

   echo "Starting writers"
   for (( a=1; a<=$WRITERS; a++ ))
   do
      ./agent write &
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

   ./agent resetiter
done

# Calculate average stats after all iterations finish
if [ $STATARG == "stat" ]
then
   ./agent stat
else
   printf "%s" `./agent $STATARG` 1>&2
fi

# Exit code 0 if no reader has expired
./agent expired
EC=$?

# Cleanup /dev/shm
./agent unlink

exit $EC