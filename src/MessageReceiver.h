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

#include "common.h"
#include <boost/asio.hpp>

namespace sienaplus {

class MessageReceiver {
public:
	MessageReceiver(boost::asio::io_service&, const std::function<void(const char*, int)>&);
	virtual ~MessageReceiver();
	virtual void start() = 0;
	virtual void stop() = 0;
protected:
	connection_type connection_type_;
	bool flag_runing_;
	std::function<void(const char*, int size)> receive_handler_;
	boost::asio::io_service& io_service_;
};

} /* namespace sienaplus */
#endif /* MESSAGERECEIVER_H_ */
