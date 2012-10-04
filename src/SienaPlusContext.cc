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
#include "sff.bzr/simple_fwd_types.h"
#include <thread>

namespace sienaplus {

SienaPlusContext::SienaPlusContext(const string& id, const string& url, std::function<void(const simple_message&)> h) {
    local_id_ = id;
	flag_has_subscription = false;
	url_ = url;
	app_notification_handler_ = h;
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
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		connection_type = sienaplus::connection_type::tcp;
	} else if(tokens[0] == "ka") {
		net_connection_ = make_shared<TCPNetworkConnector>(io_service_,
				std::bind(&SienaPlusContext::receive_handler, this,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		connect();
		connection_type = sienaplus::connection_type::ka;
	} else {
		log_err("Malformed URL or method not supported:" << url_);
		exit(-1);
	}
}

SienaPlusContext::~SienaPlusContext() {
}

void SienaPlusContext::publish(const string& msg) {
    //TODO:
    throw SienaPlusException("SienaPlusContext::publish(): not implemented.");
}

void SienaPlusContext::publish(const simple_message& msg) {
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
    // FIXME: this test is expensive ...
    char arr_buf[MAX_MSG_SIZE];
    assert(msg.SerializeToArray(arr_buf, MAX_MSG_SIZE));

    // seriazlie the message to a buffer ...
    //Send out the buffer
    //TODO: i'm sute this is very inefficient
    string str = msg.SerializeAsString();
    net_connection_->send(str.c_str(), str.length());
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

void SienaPlusContext::subscribe(const simple_filter& filtr) {
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
}

void SienaPlusContext::stop() {
	//work_->reset();
    net_connection_->disconnect();
}

void SienaPlusContext::receive_handler(NetworkConnector* nc, const char* data, int size) {
    log_debug("SienaPlusContext::receive_handler: received " << size << " bytes.");
    SienaPlusMessage buff;
	if(!buff.ParsePartialFromArray(data, size)) {
		log_err("SienaPlusContext::receive_handler(): unable to deserialize message.");
		return;
	}
    switch(buff.type()) {
    case SienaPlusMessage_message_type_t_NOT: {
        simple_message msg;
        to_simple_message(buff, msg);
        log_debug("SienaPlusContext::receive_handler(): dispatching notification to the application.");
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
	log_debug("Context connected to " << address_);
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
