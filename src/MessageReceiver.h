/**
 * @file MessageReceiver.h
 * @brief Base class for message receivers.
 * @author Amir Malekpour
 * @version 0.1
 *
 * Copyright © 2012 Amir Malekpour
 *
 *  Mana is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mana is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. For more details see the GNU General Public License
 *  at <http: *www.gnu.org/licenses/>
 */

#ifndef MESSAGERECEIVER_H_
#define MESSAGERECEIVER_H_

#include <string>
#include <boost/asio.hpp>
#include "MessageStream.h"
#include "common.h"
#include "URL.h"

namespace mana {

using namespace std;

template <class T>
class TCPMessageReceiver;

template <class T>
class UDPMessageReceiver;


template <class T>
class MessageReceiver {

public:

/** @brief Constructor
 * @param srv An instance of boost::asio::io_service;
 * @param client An object the implements a handle_message method to receive messages
 * @param port Server's port (TCP or UDP)
 * @param addr The local address of the server
 */
MessageReceiver(boost::asio::io_service& srv, T& c, const URL& url) :
    io_service_(srv), client_(c), url_(url), read_hndlr_strand_(srv),
    flag_runing_(false) {}

virtual ~MessageReceiver() {
}

boost::asio::io_service& io_service() {
	return io_service_;
}

bool is_runing() const {
	return flag_runing_;
}

const URL& url() const {
	return url_;
}

static shared_ptr<MessageReceiver<T>> create(boost::asio::io_service& srv, T& c, const URL& url) {
	if(url.protocol() == mana::connection_type::tcp) {
		return make_shared<TCPMessageReceiver<T>>(srv, c, url);
	} else if(url.protocol() == mana::connection_type::ka) {
		return make_shared<TCPMessageReceiver<T>>(srv, c, url);
	}
	assert(url.protocol() == mana::connection_type::udp);
	return make_shared<UDPMessageReceiver<T>>(srv, c, url);
}

virtual void start() = 0;
virtual void stop() = 0;
virtual connection_type transport_type() const = 0;

protected:

//methods

// properties
boost::asio::io_service& io_service_;
T& client_;
const URL url_;
boost::asio::strand read_hndlr_strand_;
array<byte, MAX_MSG_SIZE> read_buffer_;
MessageStream message_stream_;
mutex read_buff_mutex_;
connection_type connection_type_;
bool flag_runing_;

};

} /* namespace mana */
#endif /* MESSAGERECEIVER_H_ */
