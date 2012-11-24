/*
 * SienaPlusContext.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>
#include "SienaPlusContext.h"
#include "SienaPlusException.h"
#include "TCPNetworkConnector.h"
#include "common.h"
#include "ManaFwdTypes.h"
#include <thread>

namespace sienaplus {

SienaPlusContext::SienaPlusContext(const string& id, const string& url, std::function<void(const mana_message&)> h) :
    local_id_(id), url_(url), app_notification_handler_(h), flag_has_subscription(false), flag_session_established_(false) {
    vector<string> tokens;
	boost::split(tokens, url_, boost::is_any_of(":"));
	address_ = tokens[1];
	try {
		port_ = stoi(tokens[2]);
	} catch(exception& e) {
		throw SienaPlusException("Invalid port number: " + tokens[2]);
	}

	if(tokens[0] == "tcp") {
		net_connection_ = make_shared<TCPNetworkConnector>(io_service_,
				std::bind(&SienaPlusContext::receive_handler, this,
				std::placeholders::_1, std::placeholders::_2));
		connection_type = sienaplus::connection_type::tcp;
	} else if(tokens[0] == "ka") {
		net_connection_ = make_shared<TCPNetworkConnector>(io_service_,
				std::bind(&SienaPlusContext::receive_handler, this,
				std::placeholders::_1, std::placeholders::_2));
        if(!connect())
		    throw SienaPlusException("Could not connect to broker.");
		connection_type = sienaplus::connection_type::ka;
	} else {
		log_err("Malformed URL or method not supported:" << url_);
		exit(-1);
	}
}

SienaPlusContext::~SienaPlusContext() {
    stop();
}

void SienaPlusContext::publish(const string& msg) {
    //TODO:
    throw SienaPlusException("SienaPlusContext::publish(): not implemented.");
}

void SienaPlusContext::publish(const mana_message& msg) {
    SienaPlusMessage buff;
    // set the sender id
    buff.set_sender(local_id_);
    // fill in the rest of the message parts
    to_protobuf(msg, buff);
    // make sure required fields are filled in
    assert(buff.IsInitialized());
    send_message(buff);
}

void SienaPlusContext::send_message(SienaPlusMessage& msg) {
    net_connection_->send(msg);
}

/*
 * @notification_handler is the function pointer/functor that is
 * called to handle the notification
 */
void SienaPlusContext::subscribe(const string& str) {
	//if(connection_type == sienaplus::connection_type::tcp || connection_type == sienaplus::connection_type::ka)
	//net_connection_->send(str.c_str(), str.length());
    throw SienaPlusException("not implemented");
}

void SienaPlusContext::subscribe(const mana_filter& filtr) {
    SienaPlusMessage buff;

    // set sender id
    buff.set_sender(local_id_);
    // fill in the constraints : type, name, operator, value
    to_protobuf(filtr, buff);

    //make sure all required fields are filled
    assert(buff.IsInitialized());
    assert(buff.has_subscription());

    send_message(buff);
}

void SienaPlusContext::unsubscribe() {
	if(!flag_has_subscription) {
		return;
	}
}

void SienaPlusContext::start() {
	thread_ = make_shared<thread>([&]() {
		work_ = make_shared<boost::asio::io_service::work>(io_service_);
		io_service_.run();
	});
    // TODO: we should use heartbeat messages
    flag_session_established_  = true;
}

void SienaPlusContext::stop() {
	//work_->reset();
    net_connection_->disconnect();
	io_service_.stop();
    if(thread_->joinable())
        thread_->join();
    flag_session_established_ = false;
}

bool SienaPlusContext::session_established() const {
    return flag_session_established_;
}

void SienaPlusContext::receive_handler(NetworkConnector* nc, SienaPlusMessage& buff) {
    log_debug("\nSienaPlusContext::receive_handler: received message from " << buff.sender());
    switch(buff.type()) {
    case SienaPlusMessage_message_type_t_NOT: {
        mana_message msg;
        to_mana_message(buff, msg);
        log_debug("\nSienaPlusContext::receive_handler(): dispatching notification to the application.");
        app_notification_handler_(msg);
        break;
    }
    //TODO : other types ...
    default:
        log_err("SienaPlusContext::receive_handler(): unrecognized message type: " << buff.type());
    }
}

bool SienaPlusContext::connect() {
	if(is_connected())
		return true;
	if(!(net_connection_->connect(address_, port_))) {
		return false;
	}
	log_debug("\nContext connected to " << address_);
	return true;
}

bool SienaPlusContext::is_connected() {
	if(net_connection_ == nullptr || net_connection_->is_connected() == false)
		return false;
	return true;
}

void SienaPlusContext::join() {
	thread_->join();
}

} /* namespace sienaplus */
