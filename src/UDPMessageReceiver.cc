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
		const URL& url) : MessageReceiver<T>(srv, client, url) {
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
