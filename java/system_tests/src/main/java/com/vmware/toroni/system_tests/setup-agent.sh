#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

P=`readlink -f ../../../../../../../../toroni`
CLASS_PATH="$P/target/toroni-1.0.jar:$P/target/test-classes:$HOME/.m2/repository/net/java/dev/jna/jna-platform/5.12.1/jna-platform-5.12.1.jar:$HOME/.m2/repository/net/java/dev/jna/jna/5.12.1/jna-5.12.1.jar"

export SYSTEM_TESTS_ROOT=`readlink -f ../../../../../../../../../cpp/system_tests`

export TORONI_AGENT_DEFAULT="java -cp $CLASS_PATH toroni.system_tests.Agent"
export TORONI_AGENT_READER=$TORONI_AGENT_DEFAULT
export TORONI_AGENT_WRITER=$TORONI_AGENT_DEFAULT