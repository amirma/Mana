/*

 * NetworkConnectorr.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "NetworkConnector.h"

namespace mana {

template <class T>
NetworkConnector<T>::NetworkConnector(boost::asio::io_service& srv, T& c) :
	    io_service_(srv), client_(c), port_(0), address_(""),
            read_hndlr_strand_(srv), write_hndlr_strand_(srv), flag_is_connected(false),
            flag_write_op_in_prog_(false) {}

template <class T>
NetworkConnector<T>::~NetworkConnector() {}

} /* namespace mana */
