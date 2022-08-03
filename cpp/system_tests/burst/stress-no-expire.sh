#!/bin/bash

# Transfer 70MB 7*500*20K over 64KB ring buffer and 16 readers
export TORONI_AGENT_RINGBUF_SIZE_KB=64
export TORONI_AGENT_MESSAGE_SIZE_BYTES=500
export TORONI_AGENT_BACKPRESSURE_SLEEP_MS=5 # With smaller sleep readers usually expire
export TORONI_AGENT_EXTRESULT=1

./bench-cell.sh 7 7 result 10