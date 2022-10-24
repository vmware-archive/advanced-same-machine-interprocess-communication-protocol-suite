#
# Copyright 2022 VMware, Inc.
# SPDX-License-Identifier: Apache-2.0
#

add_executable(
  agent
  agent.cpp
)
set(AGENT_LIBS toroni pthread rt)

find_package(Boost)
if(Boost_FOUND)
   find_package(Boost COMPONENTS log log_setup)
   target_compile_definitions(agent PRIVATE BOOST_FOUND=1)
   set(AGENT_LIBS ${AGENT_LIBS} Boost::log Boost::log_setup)
endif()

target_link_libraries(agent PUBLIC ${AGENT_LIBS})

configure_file(bench-cell.sh bench-cell.sh COPYONLY)
configure_file(bench-table.sh bench-table.sh COPYONLY)
configure_file(stress-no-expire.sh stress-no-expire.sh COPYONLY)
configure_file(robust-writer.sh robust-writer.sh COPYONLY)
configure_file(robust-reader.sh robust-reader.sh COPYONLY)
configure_file(setup-agent.sh setup-agent.sh COPYONLY)

add_test(NAME SystemTests/StressNoExpire COMMAND ./stress-no-expire.sh)
add_test(NAME SystemTests/RobustWriter COMMAND ./robust-writer.sh)
add_test(NAME SystemTests/RobustReader COMMAND ./robust-reader.sh)