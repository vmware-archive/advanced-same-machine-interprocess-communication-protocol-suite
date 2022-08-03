#
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0
#

add_library(toroni INTERFACE)
target_compile_features(toroni INTERFACE cxx_std_17)
target_include_directories(toroni INTERFACE include)

option(p_strict "Strict warnings" OFF)
if (p_strict)
   target_compile_options(toroni PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif (p_strict)