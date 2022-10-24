#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

# Setup agent to be used by tests

export TORONI_AGENT_READER=${TORONI_AGENT_READER:-./agent}
export TORONI_AGENT_WRITER=${TORONI_AGENT_WRITER:-./agent}
export TORONI_AGENT_DEFAULT=${TORONI_AGENT_DEFAULT:-./agent}