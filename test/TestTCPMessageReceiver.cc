/**
 * TestUDPMessageReceiver.cc
 *
 *  Created on: Feb 6, 2013
 *      Author: Amir Malekpour
 */

#include <iostream>
#include <boost/asio.hpp>
#include "TCPMessageReceiver.h"
#include "ManaMessageProtobuf.pb.h"
#include "URL.h"


using namespace std;
using namespace mana;

class MessageHandler {
public:

    void handle_message(ManaMessageProtobuf& msg, MessageReceiver<MessageHandler>* mr) {
        if(msg.has_payload()) {
            // cout << "Payload hash: " << msg.key_value_map(0).value() << endl;
            std::hash<std::string> hash_fn;
            size_t hash = hash_fn(msg.payload());
            if(to_string(hash) == msg.key_value_map(0).value())
                cout << "Payload hash verified." << endl;
        } else
            cout << "No payload" << endl;
    }
};

void termination_handler(int signum) {
    exit(0);
}

static void setup_signal_hndlr() {
    if (signal (SIGINT, termination_handler) == SIG_IGN)
     signal (SIGINT, SIG_IGN);
    if (signal (SIGHUP, termination_handler) == SIG_IGN)
     signal (SIGHUP, SIG_IGN);
    if (signal (SIGTERM, termination_handler) == SIG_IGN)
     signal (SIGTERM, SIG_IGN);
}

int main() {
    setup_signal_hndlr();
    Log::ReportingLevel() = logWARNING;
    MessageHandler hndlr;
    URL url("tcp:127.0.0.1:2350");
    boost::asio::io_service io_srv;
    TCPMessageReceiver<MessageHandler> mr(io_srv, hndlr, url);
    mr.start();
    io_srv.run();
    return 0;
}






