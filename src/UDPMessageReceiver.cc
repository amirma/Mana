/*
 * UDPMessageReceiver.cpp
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#include "UDPMessageReceiver.h"

namespace mana {

UDPMessageReceiver::UDPMessageReceiver(boost::asio::io_service& srv,
		const int port, const string& addr, const std::function<void(NetworkConnector*, ManaMessage&)>& hndlr):
		MessageReceiver(srv, hndlr) {
	connection_type_ = mana::udp;
}

UDPMessageReceiver::~UDPMessageReceiver() {
}

void UDPMessageReceiver::start() {}

void UDPMessageReceiver::stop() {}

} /* namespace mana */
