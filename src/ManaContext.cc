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

#include <thread>
#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>
#include "ManaContext.h"
#include "ManaException.h"
#include "TCPNetworkConnector.h"
#include "common.h"
#include "ManaFwdTypes.h"
#include "Utility.h"

namespace mana {

ManaContext::ManaContext(const string& id, const string& loc_url, const string& rem_url,
		std::function<void(const mana_message&)> h) :
    local_id_(id), local_url_(loc_url), remote_url_(rem_url), app_notification_handler_(h),
    flag_has_subscription(false), flag_session_established_(false), task_scheduler_(io_service_) {

	session_ = make_shared<Session<ManaContext>>(*this, local_url_, remote_url_,
	    	nullptr, nullptr, remote_url_.url(), 0);
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
    session_->send(msg);
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
}

void ManaContext::handle_session_termination(Session<ManaContext>& s) {
    log_info("Session to " << s.remote_id() << " terminated.");
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

void ManaContext::handle_message(NetworkConnector<ManaContext>& nc, ManaMessage& buff) {
    log_debug("\nManaContext::receive_handler: received message from " << buff.sender());
    switch(buff.type()) {
    case ManaMessage_message_type_t_NOT: {
        mana_message msg;
        to_mana_message(buff, msg);
        log_debug("\nManaContext::receive_handler(): dispatching notification to the application.");
        app_notification_handler_(msg);
        break;
    }
    case ManaMessage_message_type_t_HEARTBEAT: {
    	log_info("Received hearbeat from " << buff.sender());
    	session_->update_hb_reception_ts();
    	break;
    }
    default:
        log_err("ManaContext::receive_handler(): unrecognized message type: " << buff.type());
        break;
    }
}

const string& ManaContext::id() const {
	return local_id_;
}

bool ManaContext::is_connected() {
	return session_->is_active();
}

void ManaContext::join() {
	thread_->join();
}

boost::asio::io_service& ManaContext::io_service() {
    return io_service_;
}

} /* namespace mana */
