#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0
set -e

# Test that
# - a writer that crashes holding the RingBuffer lock
# - another writer can write messages and reader receives them

source setup-agent.sh

export TORONI_AGENT_TEST_FLAVOR=ROBUST_WRITER # Causes writer to exit on bp and reaer block forever
export TORONI_AGENT_RINGBUF_SIZE_KB=1
export TORONI_AGENT_MESSAGES_PER_WRITER=2 # 2 messages should overflow the RingBuf
export TORONI_AGENT_MESSAGE_SIZE_BYTES=1000
export TORONI_AGENT_READERS_MAX=16
export TORONI_AGENT_READERS=1
export TORONI_AGENT_WRITERS=1

$TORONI_AGENT_DEFAULT init

# This reader will block forever on reading
$TORONI_AGENT_READER read &
r1pid=$!

# This writer will exit on backpressure holding robust lock
$TORONI_AGENT_WRITER write

# Get the blocked reader out of the way
kill $r1pid

# Assert 0 messages are read so far
$TORONI_AGENT_DEFAULT cmprcved 0

export TORONI_AGENT_TEST_FLAVOR=LATENCY # Causes writer to write and reader to read

# Start a reader that will read messages
$TORONI_AGENT_READER read &
r2pid=$!

# Wait for it to start
sleep 1s

$TORONI_AGENT_WRITER write

# Wait for reader to get messages
wait $r2pid

# Assert 2 messages are read
$TORONI_AGENT_DEFAULT cmprcved $TORONI_AGENT_MESSAGES_PER_WRITER
EC=$?

# Clean up /dev/shm
$TORONI_AGENT_DEFAULT unlink

exit $EC