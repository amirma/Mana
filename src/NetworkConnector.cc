/*
 * NetworkConnectorr.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "NetworkConnector.h"

namespace sienaplus {

NetworkConnector::NetworkConnector(boost::asio::io_service& srv, const std::function<void(NetworkConnector*, SienaPlusMessage&)>& hndlr) :
		io_service_(srv), receive_handler(hndlr), read_hndlr_strand_(io_service_), write_hndlr_strand_(io_service_),
        port_(0), address_(""), flag_is_connected(false), flag_write_op_in_prog_(false) {}

NetworkConnector::~NetworkConnector() {
}

} /* namespace sienaplus */
