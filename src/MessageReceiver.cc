/*
 * MessageReceiver.cpp
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#include "MessageReceiver.h"

namespace mana {

template <class T>
MessageReceiver<T>::MessageReceiver(boost::asio::io_service& srv, T& c,
		const int port, const string& add) : io_service_(srv), client_(c) ,
		port_(port), address_(add) {}

template <class T>
MessageReceiver<T>::~MessageReceiver() {
}

template <class T>
bool MessageReceiver<T>::is_runing() const {
	return flag_runing_;
}
} /* namespace mana */



