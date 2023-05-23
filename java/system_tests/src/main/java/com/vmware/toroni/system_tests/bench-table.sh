#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

# Java needs time (more messages and memory) to warm up. As readers shouldn't expire, prvoide large buffer
# and backpressure timeout for a large number of messages.
export TORONI_AGENT_RINGBUF_SIZE_KB=${TORONI_AGENT_RINGBUF_SIZE_KB:-65536}
export TORONI_AGENT_MESSAGES_PER_WRITER=${TORONI_AGENT_MESSAGES_PER_WRITER:-2000000}
export TORONI_AGENT_MESSAGE_SIZE_BYTES=${TORONI_AGENT_MESSAGE_SIZE_BYTES:-1}
export TORONI_AGENT_BACKPRESSURE_SLEEP_MS=${TORONI_AGENT_BACKPRESSURE_SLEEP_MS:-200}
export TORONI_AGENT_TEST_FLAVOR=${TORONI_AGENT_TEST_FLAVOR:-"FIRST_LAST_DURATION"}
export TORONI_AGENT_ITERATIONS=${TORONI_AGENT_ITERATIONS:-1}

(source ./setup-agent.sh && cd $SYSTEM_TESTS_ROOT/burst && ./bench-table.sh)