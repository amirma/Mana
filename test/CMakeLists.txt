SET (CMAKE_CXX_FLAGS "-std=c++11 -g -O2")
#
include_directories(/opt/include/)
include_directories(${MANA_SOURCE_DIR}/src/)
link_directories(/opt/lib)
#
set(SOURCES ${MANA_SOURCE_DIR}/src/ManaMessage.pb.cc ${MANA_SOURCE_DIR}/src/ManaException.cc ${MANA_SOURCE_DIR}/src/URL.cc ${MANA_SOURCE_DIR}/src/ManaContext.cc
${MANA_SOURCE_DIR}/src/ProtobufToFromMana.cc ${MANA_SOURCE_DIR}/src/MessageStream.cc ${MANA_SOURCE_DIR}/src/Utility.cc
${MANA_SOURCE_DIR}/src/StateMachine.cc)
set(LIBRARIES sff boost_system boost_thread-mt pthread protobuf profiler)

add_executable (TestUDPMessageReceiver TestUDPMessageReceiver.cc ${SOURCES})
target_link_libraries (TestUDPMessageReceiver ${LIBRARIES})

add_executable (TestUDPMessageSender TestUDPMessageSender.cc ${SOURCES})
target_link_libraries (TestUDPMessageSender ${LIBRARIES})

add_executable (TestTCPMessageSender TestTCPMessageSender.cc ${SOURCES})
target_link_libraries (TestTCPMessageSender ${LIBRARIES})

add_executable (TestTCPMessageReceiver TestTCPMessageReceiver.cc ${SOURCES})
target_link_libraries (TestTCPMessageReceiver ${LIBRARIES})

add_executable (TestClient TestClient.cc ${MANA_SOURCE_DIR}/src/SimpleClient.cc ${SOURCES})
target_link_libraries (TestClient ${LIBRARIES})

add_executable (TestClientInput TestClientInput.cc ${MANA_SOURCE_DIR}/src/SimpleClient.cc ${SOURCES})
target_link_libraries (TestClientInput ${LIBRARIES})

ENABLE_TESTING()
ADD_TEST(TestTCPMessageSenderReceiver testTCPMessageSenderReceiver.sh)
ADD_TEST(TestUDPMessageSenderReceiver testUDPMessageSenderReceiver.sh)
