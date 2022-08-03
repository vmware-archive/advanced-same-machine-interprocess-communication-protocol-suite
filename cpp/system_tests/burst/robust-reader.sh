#!/bin/bash

set -e

# Test that
# - a reader crashes holding the only ReaderInfo lock
# - another reader can acquire it and receive messages

export TORONI_AGENT_TEST_FLAVOR=ROBUST_READER # Causes reader to exit on reading
export TORONI_AGENT_MESSAGES_PER_WRITER=2
export TORONI_AGENT_READERS_MAX=1 # Have only one reader info
export TORONI_AGENT_READERS=1
export TORONI_AGENT_WRITERS=1

./agent init

# This reader will crash
./agent read

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