/*
 * TestUDPMessageReceiver.cc
 *
 *  Created on: Feb 6, 2013
 *      Author: Amir Malekpour
 */

#include <boost/asio.hpp>
#include <iostream>
#include <random>
#include <thread>
#include "TCPMessageSender.h"
#include "ManaMessage.pb.h"
#include "URL.h"
#include "Log.h"

using namespace std;
using namespace mana;

class MessageHandler {
public:
	void handle_message(ManaMessage& msg) {}
};

int main() {
	Log::ReportingLevel() = logWARNING;
	URL url("tcp:127.0.0.1:2350");
	boost::asio::io_service io_srv;
	boost::asio::io_service::work work(io_srv); // disallow the io_service to quit too soon.
	thread t([&]{io_srv.run();});
	t.detach();
	MessageHandler hndlr;
	TCPMessageSender<MessageHandler> ms(io_srv, hndlr, url);
	try {
		ms.connect();
	} catch(std::exception& e) {
		cout << "Could not connect to " << url.url() << endl;
		exit(-1);
	}
	// create a message and fill in some fields
	std::hash<std::string> hash_fn;
	const int num_chars = 1000;
	char payload[num_chars+1];
	payload[num_chars] = 0; // null-ended string
	ManaMessage msg;
	for(int k = 0; k < 10; k++) {
		msg.Clear();
		msg.set_sender("test sender");
		msg.set_type(ManaMessage_message_type_t_HEARTBEAT);
		// prepare a big payload, send the payload with its hash to the receiver
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(1, 254);
		for(int i = 0; i < num_chars; i++)
			payload[i] = dis(gen);
		auto hash = hash_fn(payload);
		msg.set_payload(payload);
		auto p = msg.mutable_key_value_map()->Add();
		p->set_key("hash");
		p->set_value(to_string(hash));
		ms.send(msg);
	}
	sleep(1);
	return 0;
}
