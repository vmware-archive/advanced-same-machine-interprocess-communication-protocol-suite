target_sources(unit_test
PRIVATE
   mpscMessageQueueTest.cpp
   serialWorkQueueTest.cpp)

target_link_libraries(unit_test PUBLIC rt)