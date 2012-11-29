/*
 * MessageReceiver.h
 * This is only a base class to define interface for
 * TCP and UDP MessageReceivers
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef MESSAGERECEIVER_H_
#define MESSAGERECEIVER_H_

#include <boost/asio.hpp>
#include <string>
#include "common.h"

namespace mana {

class NetworkConnector;

class MessageReceiver {
public:
	MessageReceiver(boost::asio::io_service&, const std::function<void(NetworkConnector*, ManaMessage&)>&);
	virtual ~MessageReceiver();
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual bool is_runing() const;
protected:
	boost::asio::io_service& io_service_;
	std::function<void(NetworkConnector*, ManaMessage&)> receive_handler_;
	connection_type connection_type_;
	bool flag_runing_;
	int port_;
	std::string address_;

};

} /* namespace mana */
#endif /* MESSAGERECEIVER_H_ */
