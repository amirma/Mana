/*
 * MessageReceiver.h
 * This is only a base class to define interface for
 * TCP and UDP MessageReceivers
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
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
