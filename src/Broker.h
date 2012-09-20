/*
 * Broker.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <boost/asio.hpp>
#include "MessageReceiver.h"
#include "SienaPlusMessage.pb.h"
#include <array>

using namespace std;

namespace sienaplus {

class Broker {
public:
	Broker();
	virtual ~Broker();
	void start();
	void shutdown();
	boost::asio::io_service& io_service();

	// Use this method to add more transport protocols to the broker
	void add_transport(string);
	void handle_message(SienaPlusMessage&);
private:
	boost::asio::io_service io_service_;
	vector<shared_ptr<MessageReceiver> > message_receivers;
	//data, size
	void receive_handler(const char*, int);
};

} /* namespace sienaplus */
#endif /* BROKER_H_ */
