/*
 * UDPMessageReceiver.cpp
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#include "UDPMessageReceiver.h"

namespace mana {

template <class T>
UDPMessageReceiver<T>::UDPMessageReceiver(boost::asio::io_service& srv, T& client,
		const int port, const string& add) : MessageReceiver<T>(srv, client, port, add) {
	this->connection_type_ = mana::udp;
}

template <class T>
UDPMessageReceiver<T>::~UDPMessageReceiver() {
}

template <class T>
void UDPMessageReceiver<T>::start() {}

template <class T>
void UDPMessageReceiver<T>::stop() {}

} /* namespace mana */
