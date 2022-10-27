#!/bin/bash
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0

# for this to work container must be started in privileged mode
route add -net 224.0.0.0 netmask 240.0.0.0 dev lo
ifconfig lo multicast