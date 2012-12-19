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
#include "common.h"
#include "URL.h"

namespace mana {

using namespace std;

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
    io_service_(srv), client_(c), url_(url) {}

virtual ~MessageReceiver() {
}

bool is_runing() const {
	return flag_runing_;
}

virtual void start() = 0;
virtual void stop() = 0;

protected:
    // methods
    // properties
    boost::asio::io_service& io_service_;
    T& client_;
    const URL url_;
    connection_type connection_type_;
    bool flag_runing_;
};

} /* namespace mana */
#endif /* MESSAGERECEIVER_H_ */
