SET (CMAKE_CXX_FLAGS "-std=c++11 -g -O2")
#
include_directories(/opt/include/)
include_directories(${MANA_SOURCE_DIR}/src/)
link_directories(/opt/lib)
#
set(LIBRARIES sff boost_system pthread protobuf profiler mana)

add_executable (subscriber subscriber.cc)
target_link_libraries (subscriber ${LIBRARIES})

add_executable (publisher publisher.cc)
target_link_libraries (publisher ${LIBRARIES})
