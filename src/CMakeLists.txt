SET (CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")
#SET (CMAKE_CXX_FLAGS "-std=c++ -DNDEBUG -g -O2")
#
include_directories(/opt/include/)
link_directories(/opt/lib)

set(SOURCES ManaMessageProtobuf.pb.cc ManaException.cc URL.cc
ProtobufToFromMana.cc MessageStream.cc Utility.cc
StateMachine.cc ManaContext.cc)

set(LIBRARIES sff boost_system boost_program_options pthread protobuf profiler)
#
add_library(mana ${SOURCES})
target_link_libraries (mana ${LIBRARIES})


add_executable (StartBroker StartBroker.cc Broker.cc)
target_link_libraries (StartBroker ${LIBRARIES} mana)

# This target is to generate protocol buffers classes from the protobuf.
#add_custom_target(protobuf protoc -I . --cpp_out . ManaMessageProtobuf.proto)
