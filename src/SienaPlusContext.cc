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

SienaPlusContext::SienaPlusContext(const string& url, std::function<void(const simple_message&)> h) {
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

    SienaPlusMessage_subscription_t* s = msg.mutable_subscription();
    for(auto& cnst : filtr) {
    	SienaPlusMessage_subscription_t_constraint_t* c = s->add_constraints();
    	c->set_name(cnst.name().c_str());
    	c->mutable_value()->set_type(SienaPlusMessage_tag_type_t(cnst.type()));
    	c->set_op(SienaPlusMessage_subscription_t_operator_t(cnst.op()));
        switch(cnst.type()) {
            case siena::type_id::string_id:
            	c->mutable_value()->set_string_value(cnst.string_value().c_str());
                break;
            case siena::type_id::int_id:
            	c->mutable_value()->set_int_value(cnst.int_value());
                break;
            case siena::type_id::double_id:
            	c->mutable_value()->set_double_value(cnst.double_value());
                break;
            case siena::type_id::bool_id:
            	c->mutable_value()->set_bool_value(cnst.bool_value());
                break;
            case siena::type_id::anytype_id:
            	cout << "Type any is not supported yet.";
                break;
       }

        //make sure all required fields are filled
        assert(c->has_name());
        assert(c->has_op());
        assert(c->has_value());
    }

    //make sure all required fields are filled
    assert(msg.has_type());
    assert(msg.has_sender());
    assert(msg.has_subscription());

    //TOOD here the buffer should come from a pool
    char arr_buf[MAX_MSG_SIZE];
    if(!msg.SerializePartialToArray(arr_buf, MAX_MSG_SIZE)) {
    	cout << "SienaPlusContext::subscribe: unable to serialize message.";
    	return;
    }

    //SienaPlusMessage msg1;
    //assert(msg1.ParsePartialFromArray(arr_buf, MAX_MSG_SIZE));
    //assert(msg1.ParseFromString(msg.SerializeAsString()));


   //Send out the buffer
   string str = msg.SerializeAsString();
   net_connection_->send(str.c_str(), str.length());
   cout << endl << str.c_str();
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
