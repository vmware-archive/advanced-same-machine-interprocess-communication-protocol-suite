#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

set -e

# Test that
# - a writer that crashes holding the RingBuffer lock
# - another writer can write messages and reader receives them

export TORONI_AGENT_TEST_FLAVOR=ROBUST_WRITER # Causes writer to exit on bp and reaer block forever
export TORONI_AGENT_RINGBUF_SIZE_KB=1
export TORONI_AGENT_MESSAGES_PER_WRITER=2 # 2 messages should overflow the RingBuf
export TORONI_AGENT_MESSAGE_SIZE_BYTES=1000
export TORONI_AGENT_READERS_MAX=16
export TORONI_AGENT_READERS=1
export TORONI_AGENT_WRITERS=1

./agent init

# This reader will block forever on reading
./agent read &
r1pid=$!

# This writer will exit on backpressure holding robust lock
./agent write

# Get the blocked reader out of the way
kill $r1pid

# Assert 0 messages are read so far
./agent cmprcved 0

export TORONI_AGENT_TEST_FLAVOR=LATENCY # Causes writer to write and reader to read

# Start a reader that will read messages
./agent read &
r2pid=$!

# Wait for it to start
sleep 1s

./agent write

# Wait for reader to get messages
wait $r2pid

# Assert 2 messages are read
./agent cmprcved $TORONI_AGENT_MESSAGES_PER_WRITER
EC=$?

# Clean up /dev/shm
./agent unlink

exit $EC