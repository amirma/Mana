/**
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
#include "MessageReceiver.h"
#include "TCPMessageReceiver.h"
#include "UDPMessageReceiver.h"
#include "Session.h"
#include "common.h"
#include "ManaFwdTypes.h"
#include "Utility.h"
#include "Log.h"

namespace mana {

ManaContext::ManaContext(const string& id, const string& loc_url, const string& rem_url,
		std::function<void(const ManaMessage&)> h) :
    local_id_(id), local_url_(loc_url), remote_url_(rem_url), app_notification_handler_(h),
    flag_has_subscription(false), task_scheduler_(io_service_) {

	session_ = make_shared<Session<ManaContext>>(*this, local_url_, remote_url_, remote_url_.url(), 0);
	message_receiver_ = MessageReceiver<ManaContext>::create(io_service_, *this, local_url_);
	assert(message_receiver_ != nullptr);
}

ManaContext::~ManaContext() {
    stop();
}

void ManaContext::publish(const string& str) {
    ManaMessage msg;
    string_to_ManaMessage(str, msg);
    publish(msg);
}

void ManaContext::publish(const ManaMessage& msg) {
    ManaMessageProtobuf buff;
    // set the sender id
    buff.set_sender(local_id_);
    // fill in the rest of the message parts
    to_protobuf(msg, buff);
    // make sure required fields are filled in
    assert(buff.IsInitialized());
    send_message(buff);
}

void ManaContext::send_message(ManaMessageProtobuf& msg) {
    session_->send(msg);
}

/*
 * @notification_handler is the function pointer/functor that is
 * called to handle the notification
 */
void ManaContext::subscribe(const string& str) {
    ManaFilter f;
    string_to_ManaFilter(str, f);
    subscribe(f);
}

void ManaContext::subscribe(const ManaFilter& filtr) {
    // TODO: here we need to check the sanity of the filter
    // in different aspects:
    // 1. If the operators make sense for the supplied values
    // 2. If a given constraint has a consistent value types among
    // different filters of this subscriber.

    ManaMessageProtobuf buff;
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
    message_receiver_->start();
    session_->establish();
}

void ManaContext::handle_session_termination(Session<ManaContext>& s) {
	// TODO ...
	FILE_LOG(logINFO) << "Session to " << s.remote_id() << " terminated.";
}

void ManaContext::stop() {
	//work_->reset();
    session_->terminate();
    // FIXME: this will probably cause a bug, because stopping io_service
    // might happen before termination message is sent.
	io_service_.stop();
    if(thread_->joinable())
        thread_->join();
}

bool ManaContext::session_established() const {
    return session_->is_active();
}

void ManaContext::handle_message(const ManaMessageProtobuf& buff, MessageReceiver<ManaContext>* mr) {
	FILE_LOG(logDEBUG2) << "ManaContext::receive_handler: received message from " << buff.sender();
    switch(buff.type()) {
    case ManaMessageProtobuf_message_type_t_NOT: {
        ManaMessage msg;
        to_ManaMessage(buff, msg);
        FILE_LOG(logDEBUG2) << "ManaContext::receive_handler(): dispatching notification to the application.";
        app_notification_handler_(msg);
        break;
    }
    case ManaMessageProtobuf_message_type_t_HEARTBEAT:
    case ManaMessageProtobuf_message_type_t_START_SESSION:
    case ManaMessageProtobuf_message_type_t_START_SESSION_ACK :
    case ManaMessageProtobuf_message_type_t_START_SESSION_ACK_ACK:
    case ManaMessageProtobuf_message_type_t_TERMINATE_SESSION :
    case ManaMessageProtobuf_message_type_t_TERMINATE_SESSION_ACK :
    	session_->handle_session_msg(buff);
    	break;
    default:
    	FILE_LOG(logWARNING) << "ManaContext::receive_handler(): unrecognized message type: " << buff.type();
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
