/*
 * NetworkConnectorr.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "NetworkConnector.h"

namespace sienaplus {

NetworkConnector::NetworkConnector(boost::asio::io_service& srv, std::function<void(const char*, int size)> hndlr) :
		io_service_(srv), strand_(io_service_) {
	receive_handler = hndlr;
	port_ = 0;
	address_ = "";
	flag_is_connected = false;
}

NetworkConnector::~NetworkConnector() {
}

} /* namespace sienaplus */
