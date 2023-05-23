#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

(source ./setup-agent.sh && cd $SYSTEM_TESTS_ROOT/burst && ./robust-writer.sh)