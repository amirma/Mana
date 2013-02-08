SET (CMAKE_CXX_FLAGS "-std=c++0x -DNDEBUG -g -O2")
#
include_directories(/opt/include/)
include_directories(../src/)
link_directories(/opt/lib)
#
add_executable (TestUDPMessageReceiver TestUDPMessageReceiver.cc
../src/ManaMessage.pb.cc ../src/ManaException.cc ../src/URL.cc ../src/ManaContext.cc
../src/ProtobufToFromMana.cc ../src/MessageStream.cc ../src/Utility.cc)
target_link_libraries (TestUDPMessageReceiver sff boost_system boost_thread-mt pthread protobuf profiler)

add_executable (TestUDPMessageSender TestUDPMessageSender.cc
../src/ManaMessage.pb.cc ../src/ManaException.cc ../src/URL.cc ../src/ManaContext.cc
../src/ProtobufToFromMana.cc ../src/MessageStream.cc ../src/Utility.cc)
target_link_libraries (TestUDPMessageSender sff boost_system boost_thread-mt pthread protobuf profiler)
