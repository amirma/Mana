/*
 * TestUDPMessageReceiver.cc
 *
 *  Created on: Feb 6, 2013
 *      Author: Amir Malekpour
 */

#include <iostream>
#include <boost/asio.hpp>
#include "UDPMessageReceiver.h"
#include "ManaMessage.pb.h"
#include "URL.h"


using namespace std;
using namespace mana;

class MessageHandler {
public:

	void handle_message(ManaMessage& msg) {
		cout << "Receiver message from " << msg.sender() << endl;
		if(msg.has_payload()) {
			cout << "Payload hash: " << msg.key_value_map(0).value() << endl;
			std::hash<std::string> hash_fn;
			size_t hash = hash_fn(msg.payload());
			if(to_string(hash) == msg.key_value_map(0).value())
				cout << "Payload hash verified." << endl;
		} else
			cout << "No payload" << endl;
	}
};

int main() {
	MessageHandler hndlr;
	URL url("udp:127.0.0.1:2350");
	boost::asio::io_service io_srv;
	UDPMessageReceiver<MessageHandler> mr(io_srv, hndlr, url);
	mr.start();
	io_srv.run();
	return 0;
}






