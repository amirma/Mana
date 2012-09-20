/**
 * @file Broker.cpp
 * Broker implementation
 *
 * @brief Can use "brief" tag to explicitly generate comments for file documentation.
 *
 * @author Amir Malekpour
 * @version 0.1
 */

#include "Broker.h"
#include "ConnectionType.h"
#include "SienaPlusException.h"
#include <boost/algorithm/string.hpp>
#include "MessageReceiver.h"
#include "TCPMessageReceiver.h"
#include "UDPMessageReceiver.h"

using namespace std;

namespace sienaplus {

Broker::Broker() {
}

Broker::~Broker() {
}

void Broker::add_transport(string url) {
	vector<string> tokens;
	boost::split(tokens, url, boost::is_any_of(":"));
	string& addr = tokens[1];
	int port = 0;
	try {
		port = stoi(tokens[2]);
	} catch(exception& e) {
		throw SienaPlusException("Could not connect: invalid port number: " + tokens[2]);
	}

	if(tokens[0] == "tcp") {
		auto h = std::bind(&Broker::receive_handler, this, std::placeholders::_1, std::placeholders::_2);
		//std::shared_ptr<MessageReceiver> mr(new TCPMessageReceiver(io_service_ ,port, addr, h));
		std::shared_ptr<MessageReceiver> mr(new TCPMessageReceiver(io_service_ ,port, addr, [&](const char* data,
				int size){receive_handler(data, size);}));
		message_receivers.push_back(mr);

	} else {
		cout << "Malformed URL or method not supported:" << url << endl;
		exit(-1);
	}
}

void Broker::start() {
	for(auto tr : message_receivers)
		tr->start();
	for (;;)
	{
	  try
	  {
	    io_service_.run();
	    break; // run() exited normally
	  }
	  catch (SienaPlusException& e)
	  {
	    // Deal with exception as appropriate.
	  }
	}
}

void Broker::shutdown() {
	io_service_.stop();
}

boost::asio::io_service& Broker::io_service() {
	return io_service_;
}

void Broker::handle_message(SienaPlusMessage& msg) {

}

void Broker::receive_handler(const char* data, int size) {
	cout << endl << "broker received: " << data;
	//cout << endl << "broker received: ";
}

} /* namespace sienaplus */
