SET (CMAKE_CXX_FLAGS "-std=c++0x -g -Wall")
#SET (CMAKE_CXX_FLAGS "-std=c++0x -DNDEBUG -g -O2")
#
include_directories(/opt/include/)
link_directories(/opt/lib)
set(SOURCES ../src/ManaMessage.pb.cc ../src/ManaException.cc ../src/URL.cc
../src/ProtobufToFromMana.cc ../src/MessageStream.cc ../src/Utility.cc)
set(LIBRARIES sff boost_system boost_thread-mt pthread protobuf profiler)

#
add_executable (StartBroker StartBroker.cc Broker.cc ${SOURCES})
target_link_libraries (StartBroker ${LIBRARIES})
#
add_executable (TestClient TestClient.cc SimpleClient.cc ManaContext.cc ${SOURCES})
target_link_libraries (TestClient ${LIBRARIES})

add_executable (TestClientInput TestClientInput.cc SimpleClient.cc ManaContext.cc ${SOURCES})
target_link_libraries (TestClientInput ${LIBRARIES})

# This target is to generate protocol buffers classes from the protobuf.
#add_custom_target(protobuf protoc -I . --cpp_out . ManaMessage.proto)
