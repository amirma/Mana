/*
 * @file ManaContext.cc
 * @brief Client interface to the content-based network. A
 *
 * A subscriber/publisher creates an instance of ManaContext to connect
 * to a broker, subscriber to contents and publish.
 *
 * @author Amir Malekpour
 * @version 0.1
 *
 * Copyright © 2012 Amir Malekpour
 *
 * Mana is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mana is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. For more details see the GNU General Public License
 * at <http: *www.gnu.org/licenses/>
 */

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>
#include <thread>
#include "ManaContext.h"
#include "ManaException.h"
#include "TCPNetworkConnector.h"
#include "common.h"
#include "ManaFwdTypes.h"
#include "Utility.h"

namespace mana {

ManaContext::ManaContext(const string& id, const string& url, std::function<void(const mana_message&)> h) :
    local_id_(id), url_(url), app_notification_handler_(h), flag_has_subscription(false), flag_session_established_(false) {
    vector<string> tokens;
	boost::split(tokens, url_, boost::is_any_of(":"));
	address_ = tokens[1];
	try {
		port_ = stoi(tokens[2]);
	} catch(exception& e) {
		throw ManaException("Invalid port number: " + tokens[2]);
	}

	if(tokens[0] == "tcp") {
		net_connection_ = make_shared<TCPNetworkConnector>(io_service_,
				std::bind(&ManaContext::receive_handler, this,
				std::placeholders::_1, std::placeholders::_2));
		connection_type = mana::connection_type::tcp;
	} else if(tokens[0] == "ka") {
		net_connection_ = make_shared<TCPNetworkConnector>(io_service_,
				std::bind(&ManaContext::receive_handler, this,
				std::placeholders::_1, std::placeholders::_2));
        if(!connect())
		    throw ManaException("Could not connect to broker.");
		connection_type = mana::connection_type::ka;
	} else {
		log_err("Malformed URL or method not supported:" << url_);
		exit(-1);
	}
}

ManaContext::~ManaContext() {
    stop();
}

void ManaContext::publish(const string& str) {
    mana_message msg;
    string_to_mana_message(str, msg);
    publish(msg);
}

void ManaContext::publish(const mana_message& msg) {
    ManaMessage buff;
    // set the sender id
    buff.set_sender(local_id_);
    // fill in the rest of the message parts
    to_protobuf(msg, buff);
    // make sure required fields are filled in
    assert(buff.IsInitialized());
    send_message(buff);
}

void ManaContext::send_message(ManaMessage& msg) {
    net_connection_->send(msg);
}

/*
 * @notification_handler is the function pointer/functor that is
 * called to handle the notification
 */
void ManaContext::subscribe(const string& str) {
    mana_filter f;
    string_to_mana_filter(str, f);
    subscribe(f);
}

void ManaContext::subscribe(const mana_filter& filtr) {
    // TODO: here we need to check the sanity of the filter
    // in different aspects:
    // 1. If the operators make sense for the supplied values
    // 2. If a given constraint has a consistent value types among
    // different filters of this subscriber.

    ManaMessage buff;
    // set sender id
    buff.set_sender(local_id_);
    // fill in the constraints : type, name, operator, value
    to_protobuf(filtr, buff);

    //make sure all required fields are filled
    assert(buff.IsInitialized());
    assert(buff.has_subscription());

    send_message(buff);
}

void ManaContext::unsubscribe() {
	if(!flag_has_subscription) {
		return;
	}
}

void ManaContext::start() {
	thread_ = make_shared<thread>([&]() {
		work_ = make_shared<boost::asio::io_service::work>(io_service_);
		io_service_.run();
	});
    // TODO: we should use heartbeat messages
    flag_session_established_  = true;
}

void ManaContext::stop() {
	//work_->reset();
    net_connection_->disconnect();
	io_service_.stop();
    if(thread_->joinable())
        thread_->join();
    flag_session_established_ = false;
}

bool ManaContext::session_established() const {
    return flag_session_established_;
}

void ManaContext::receive_handler(NetworkConnector* nc, ManaMessage& buff) {
    log_debug("\nManaContext::receive_handler: received message from " << buff.sender());
    switch(buff.type()) {
    case ManaMessage_message_type_t_NOT: {
        mana_message msg;
        to_mana_message(buff, msg);
        log_debug("\nManaContext::receive_handler(): dispatching notification to the application.");
        app_notification_handler_(msg);
        break;
    }
    //TODO : other types ...
    default:
        log_err("ManaContext::receive_handler(): unrecognized message type: " << buff.type());
    }
}

bool ManaContext::connect() {
	if(is_connected())
		return true;
	if(!(net_connection_->connect(address_, port_))) {
		return false;
	}
	log_debug("\nContext connected to " << address_);
	return true;
}

bool ManaContext::is_connected() {
	if(net_connection_ == nullptr || net_connection_->is_connected() == false)
		return false;
	return true;
}

void ManaContext::join() {
	thread_->join();
}

} /* namespace mana */
