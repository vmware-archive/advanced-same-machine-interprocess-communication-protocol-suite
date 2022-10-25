#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

source setup-multicast.sh

CLASS_PATH="/toroni/java/toroni/target/toroni-1.0.jar:/toroni/java/toroni/target/test-classes:$HOME/.m2/repository/net/java/dev/jna/jna-platform/5.12.1/jna-platform-5.12.1.jar:$HOME/.m2/repository/net/java/dev/jna/jna/5.12.1/jna-5.12.1.jar"
SYSTEM_TESTS_ROOT=/toroni/cpp/system_tests

export TORONI_AGENT_RINGBUF_SIZE_KB=64
export TORONI_AGENT_MESSAGE_SIZE_BYTES=500
export TORONI_AGENT_BACKPRESSURE_SLEEP_MS=15
export TORONI_AGENT_EXTRESULT=1
export TORONI_AGENT_ITERATIONS=1
export TORONI_AGENT_TEST_FLAVOR=FIRST_LAST_DURATION
export TORONI_AGENT_MESSAGES_PER_WRITER=1000

export TORONI_AGENT_DEFAULT="/cpp-burst/agent"
export TORONI_AGENT_READER="java -cp $CLASS_PATH toroni.system_tests.Agent"
export TORONI_AGENT_WRITER=$TORONI_AGENT_DEFAULT
(cd $SYSTEM_TESTS_ROOT/burst && ./bench-cell.sh 1 1 stat 1)

export TORONI_AGENT_DEFAULT="java -cp $CLASS_PATH toroni.system_tests.Agent"
export TORONI_AGENT_READER="/cpp-burst/agent"
export TORONI_AGENT_WRITER=$TORONI_AGENT_DEFAULT
(cd $SYSTEM_TESTS_ROOT/burst && ./bench-cell.sh 1 1 stat 1)