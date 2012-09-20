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
#include "ConnectionType.h"
#include "sff.bzr/simple_fwd_types.h"
#include <thread>

namespace sienaplus {

SienaPlusContext::SienaPlusContext(const string& url, std::function<void(const simple_message&)> h) {
	flag_has_subscription = false;
	url_ = url;
	handler_ = h;
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
		connect();
		connection_type = sienaplus::connection_type::ka;
	} else {
		cout << "Malformed URL or method not supported:" << url_ << endl;
		exit(-1);
	}
}

SienaPlusContext::~SienaPlusContext() {
}

void SienaPlusContext::publish() {
}

/*
 * @notification_handler is the function pointer/functor that is
 * called to handle the notification
 */
void SienaPlusContext::subscribe(const string& str) {
	if(connection_type == sienaplus::connection_type::tcp || connection_type == sienaplus::connection_type::ka)
	net_connection_->send(str.c_str(), str.length());
}

void SienaPlusContext::subscribe(const simple_filter& filtr) {
    SienaPlusMessage msg;
    // set message type
    msg.set_type(SienaPlusMessage_message_type_t_SUB);
    // set sender id
    msg.set_sender("me");
    // fill in the constraints : type, name, operator, value
    /*
    for(auto& cnst : filtr) {
        switch(cnst.type()) {
            case siena::type_id::string_id:
                break;
            case siena::type_id::int_id:
                
                break;
            case siena::type_id::double_id:

                break;
            case siena::type_id::bool_id:

                break;
            case siena::type_id::anytype_id:
                
                break;

       }
    }
    */

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
}

void SienaPlusContext::receive_handler(const char* data, int) {
	cout << endl << "context received: " << data;
}

bool SienaPlusContext::connect() {
	if(is_connected())
		return true;
	if(!(net_connection_->connect(address_, port_))) {
		return false;
	}
	cout << "Context connected to " << address_;
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
