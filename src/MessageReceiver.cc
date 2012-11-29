/*
 * MessageReceiver.cpp
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#include "MessageReceiver.h"

namespace mana {

MessageReceiver::MessageReceiver(boost::asio::io_service& srv, const std::function<void(NetworkConnector*, 
        ManaMessage&)>& hndlr) :
	io_service_(srv), receive_handler_(hndlr), flag_runing_(false) {}

MessageReceiver::~MessageReceiver() {
}

bool MessageReceiver::is_runing() const {
	return flag_runing_;
}
} /* namespace mana */



