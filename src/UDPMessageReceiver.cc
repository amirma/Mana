/*
 * UDPMessageReceiver.cpp
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#include "UDPMessageReceiver.h"

namespace sienaplus {

UDPMessageReceiver::UDPMessageReceiver(boost::asio::io_service& srv,
		const int port, const string& addr, const std::function<void(NetworkConnector*, SienaPlusMessage&)>& hndlr):
		MessageReceiver(srv, hndlr) {
	connection_type_ = sienaplus::udp;
}

UDPMessageReceiver::~UDPMessageReceiver() {
}

void UDPMessageReceiver::start() {}

void UDPMessageReceiver::stop() {}

} /* namespace sienaplus */
