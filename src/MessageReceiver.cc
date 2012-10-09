/*
 * MessageReceiver.cpp
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#include "MessageReceiver.h"

namespace sienaplus {

MessageReceiver::MessageReceiver(boost::asio::io_service& srv, const std::function<void(NetworkConnector*, 
        SienaPlusMessage&)>& hndlr) :
	io_service_(srv), receive_handler_(hndlr) {
	flag_runing_ = false;
}

MessageReceiver::~MessageReceiver() {
}
} /* namespace sienaplus */



