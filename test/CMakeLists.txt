SET (CMAKE_CXX_FLAGS "-std=c++0x -DNDEBUG -g -O0")
#
include_directories(/opt/include/)
include_directories(../src/)
link_directories(/opt/lib)
#
set(SOURCES ../src/ManaMessage.pb.cc ../src/ManaException.cc ../src/URL.cc ../src/ManaContext.cc
../src/ProtobufToFromMana.cc ../src/MessageStream.cc ../src/Utility.cc)
set(LIBRARIES sff boost_system boost_thread-mt pthread protobuf profiler)

add_executable (TestUDPMessageReceiver TestUDPMessageReceiver.cc ${SOURCES})
target_link_libraries (TestUDPMessageReceiver ${LIBRARIES})

add_executable (TestUDPMessageSender TestUDPMessageSender.cc ${SOURCES})
target_link_libraries (TestUDPMessageSender ${LIBRARIES})

add_executable (TestTCPMessageSender TestTCPMessageSender.cc ${SOURCES})
target_link_libraries (TestTCPMessageSender ${LIBRARIES})

add_executable (TestTCPMessageReceiver TestTCPMessageReceiver.cc ${SOURCES})
target_link_libraries (TestTCPMessageReceiver ${LIBRARIES})
