SET (CMAKE_CXX_FLAGS "-std=c++0x -g -Wall -fstack-protector-all")
#SET (CMAKE_CXX_FLAGS "-std=c++0x -DNDEBUG -g -O2")
#
include_directories(/opt/include/)
link_directories(/opt/lib)
#
add_executable (StartBroker StartBroker.cc Broker.cc MessageReceiver.cc
    TCPMessageReceiver.cc UDPMessageReceiver.cc SienaPlusMessage.pb.cc
    NetworkConnector.cc TCPNetworkConnector.cc SienaPlusException.cc
    SienaPlusContext.cc ProtobufToFromSiena.cc MessageStream.cc)
target_link_libraries (StartBroker sff boost_system boost_thread-mt pthread protobuf profiler)
#
add_executable (TestClient TestClient.cc SimpleClient.cc SienaPlusContext.cc TCPNetworkConnector.cc
    NetworkConnector.cc SienaPlusException.cc SienaPlusMessage.pb.cc ProtobufToFromSiena.cc
    MessageStream.cc)
target_link_libraries (TestClient sff boost_system boost_thread-mt pthread protobuf profiler)

add_executable (TestClientInput TestClientInput.cc SimpleClient.cc SienaPlusContext.cc TCPNetworkConnector.cc
    NetworkConnector.cc SienaPlusException.cc SienaPlusMessage.pb.cc ProtobufToFromSiena.cc
    MessageStream.cc)
target_link_libraries (TestClientInput sff boost_system boost_thread-mt pthread protobuf profiler)

# This target is to generate protocol buffers classes from the protobuf.
#add_custom_target(protobuf protoc -I . --cpp_out . SienaPlusMessage.proto)
