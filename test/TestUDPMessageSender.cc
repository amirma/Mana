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
#include "UDPMessageSender.h"
#include "ManaMessageProtobuf.pb.h"
#include "URL.h"
#include "Log.h"

using namespace std;
using namespace mana;

class MessageHandler {
public:
	void handle_message(ManaMessageProtobuf& msg) {}
};

int main() {
	Log::ReportingLevel() = logWARNING;
	URL url("udp:127.0.0.1:2350");
	boost::asio::io_service io_srv;
	boost::asio::io_service::work work(io_srv); // disallow the io_service to quit too soon.
	thread t([&]() {io_srv.run();});
	t.detach();
	MessageHandler hndlr;
	UDPMessageSender<MessageHandler> ms(io_srv, hndlr, url);
	// create a message and fill in some fields
	std::hash<std::string> hash_fn;
	const int num_chars = 1200;
	char payload[num_chars+1];
	payload[num_chars] = 0; // null-ended string
	ManaMessageProtobuf msg;
	for(int k = 0; k < 10; k++) {
		msg.Clear();
		msg.set_sender("test sender");
		msg.set_type(ManaMessageProtobuf_message_type_t_HEARTBEAT);
		// prepare a big payload, send the payload with its hash to the receiver
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(33, 253);
		for(int i = 0; i < num_chars; i++)
			payload[i] = dis(gen);
		auto hash = hash_fn(payload);
		msg.set_payload(payload);
		auto p = msg.mutable_key_value_map()->Add();
		p->set_key("hash");
		p->set_value(to_string(hash));
		ms.send(msg);
	}
	sleep(3);
        io_srv.stop();
        try {
            t.join();
        } catch(...){}
	return 0;
}
