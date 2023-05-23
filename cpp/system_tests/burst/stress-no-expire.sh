#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

source setup-agent.sh

# Transfer 50MB 5*500*20K over 64KB ring buffer and 16 readers
export TORONI_AGENT_RINGBUF_SIZE_KB=64
export TORONI_AGENT_MESSAGE_SIZE_BYTES=500
export TORONI_AGENT_BACKPRESSURE_SLEEP_MS=10 # With smaller sleep readers usually expire
export TORONI_AGENT_EXTRESULT=1
export TORONI_AGENT_MESSAGES_PER_WRITER=20000
export TORONI_AGENT_ITERATIONS=10
export TORONI_AGENT_TEST_FLAVOR=FIRST_LAST_DURATION

./bench-cell.sh 5 5 result