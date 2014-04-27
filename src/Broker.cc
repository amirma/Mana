/**
 * @file Broker.cc
 * Broker implementation
 *
 * @brief This class implements a messaging broker.
 *
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

#include <memory>
#include <assert.h>
#include <thread>
#include "ManaException.h"
#include "MessageReceiver.h"
#include "TCPMessageReceiver.h"
#include "UDPMessageReceiver.h"
#include "ManaFwdTypes.h"
#include "Session.h"
#include "Broker.h"
#include "TaskScheduler.h"
#include "common.h"
#include "URL.h"
#include "Log.h"

using namespace std;

namespace mana {


Broker::Broker(const string& id, size_t t) : id_(id), num_of_threads_(t),
    task_scheduler_(io_service_) {
    message_match_handler_ = new BrokerMatchMessageHandler(this);
}

Broker::~Broker() {
    delete message_match_handler_;
}

void Broker::add_transport(string str_url) {
	URL url(str_url);
	auto mr = MessageReceiver<Broker>::create(io_service_, *this, url);
    message_receivers.push_back(mr);
}

void Broker::start() {
    FILE_LOG(logINFO) << "Starting broker...";
    for(auto& tr : message_receivers)
    	tr->start();
    // make sure it least one of the transports started successfully. If not print an error
    // and exit
    bool flag = false;
    for(auto& tr : message_receivers)
        if(tr->is_runing()) {
        flag = true;
        break;
    }
    if(!flag) {
    	FILE_LOG(logERROR) << "Broker::start(): No active transport. Terminating.";
    	exit(-1);
    }
    try {
        // all threads except one get detached
        for (unsigned int i = 0; i < num_of_threads_ - 1; i++)
        std::thread([&](){io_service_.run();}).detach();
        // the last thread does not detach so we block here until the broker
        // is shutdown
        io_service_.run();
    } catch (const exception& e) {
        FILE_LOG(logERROR) << "An error happened in the broker execution: " << e.what();
    }
}

void Broker::shutdown() {
	FILE_LOG(logINFO) << "Broker is shutting down...";
	io_service_.stop();
	FILE_LOG(logINFO) << "done.";
}

boost::asio::io_service& Broker::io_service() {
   return io_service_;
}

void Broker::handle_session_initiation(const ManaMessageProtobuf& buff, const MessageReceiver<Broker>* mr) {
    // if the subscribing node is already in the list of neighbor
    // then send an error message
    if(is_in_container(neighbors_by_id_, buff.sender())) {
        send_error();
        return;
    }
    // make sure we got a properly formed message
    if(buff.key_value_map_size() < 0) {
        send_error();
        return;
    }
    try {
    	URL remote_url(buff.key_value_map(0).value());
        assert(buff.key_value_map(0).key() == "url");
        const URL& local_url = mr->url();
        // assign a new interface id
        const siena::if_t  if_no = iface_no_generator_.borrow_number();
        FILE_LOG(logDEBUG2) << "Broker::handle_session_initiation(): iface no: " << if_no;
        // make sure we got a properly formed message
        auto tmp = make_shared<Session<Broker>>(*this, local_url, remote_url, buff.sender(), if_no);
        neighbors_by_id_[buff.sender()] = tmp;
        neighbors_by_iface_[if_no] = std::move(tmp);
    } catch(const exception& e) {
        FILE_LOG(logINFO) << "Broker::handle_session_initiation(): received invalid or malformed session requesr from " << buff.sender();
        send_error();
        return;
    }
}

void Broker::handle_sub(const ManaMessageProtobuf& buff) {
    ManaFilter* fltr = new ManaFilter();
    to_ManaFilter(buff, *fltr);
    //TODO: one predicate per filter is not the right way...
    mana_predicate pred;
    pred.add(fltr);
    siena::if_t if_no;
    if(is_in_container(neighbors_by_id_, buff.sender())) {
        if_no =  neighbors_by_id_[buff.sender()]->iface();
    } else {
    	FILE_LOG(logDEBUG2) << "Broker::handle_sub: Subscription request received for unknown session. Sender id: " << buff.sender();
        send_error();
        return;
        // TODO:
        // Here we should send an error back to the client
        //
    }
    {// lock the forwarding table before inserting filters into it
        lock_guard<FwdTableWrapper> lock(fwd_table_wrapper_);
        fwd_table_wrapper_.fwd_table().ifconfig(if_no, pred);
        fwd_table_wrapper_.fwd_table().consolidate();
    }
}

void Broker::handle_not(const ManaMessageProtobuf& buff) {
    ManaMessage msg;
    to_ManaMessage(buff, msg);
    //FIXME:  this has to turn to read/write locking
    lock_guard<FwdTableWrapper> lock(fwd_table_wrapper_);
    fwd_table_wrapper_.const_fwd_table().match(msg, *message_match_handler_);
}

void Broker::handle_session_message(const ManaMessageProtobuf& buff) {
    if(is_in_container(neighbors_by_id_, buff.sender())) {
    	FILE_LOG(logDEBUG2)  << "Received hearbeat from " << buff.sender();
    	neighbors_by_id_[buff.sender()]->handle_session_msg(buff);
    }
}

/**
 * Brief: With TCP/KA transport this callback is called by the main acceptor.
 *
 * With TCP/KA transport this callback is called by the main acceptor. This
 * enables the Broker to memorize the connection for later use.
 */
void Broker::handle_connect(shared_ptr<MessageSender<Broker>>& c) {
    //TODO save a pointer to c in a new session ...
}

void Broker::handle_message(const ManaMessageProtobuf& msg, MessageReceiver<Broker>* mr) {
	switch(msg.type()) {
	case ManaMessageProtobuf_message_type_t_SUB:
		handle_sub(msg);
		break;
	case ManaMessageProtobuf_message_type_t_NOT:
		handle_not(msg);
		break;
	case ManaMessageProtobuf_message_type_t_START_SESSION:
		handle_session_initiation(msg, mr);
		break;
	case ManaMessageProtobuf_message_type_t_HEARTBEAT:
	case ManaMessageProtobuf_message_type_t_START_SESSION_ACK :
	case ManaMessageProtobuf_message_type_t_START_SESSION_ACK_ACK:
	case ManaMessageProtobuf_message_type_t_TERMINATE_SESSION :
	case ManaMessageProtobuf_message_type_t_TERMINATE_SESSION_ACK :
		handle_session_message(msg);
		break;
	default: // other types ...
		FILE_LOG(logWARNING) << "Broker::handle_message: message of invalid typed was received.";
		break;
	}
}

bool Broker::handle_match(siena::if_t iface, const siena::message& msg) {
	FILE_LOG(logDEBUG2) << "Broker::handle_match(): match for client " << neighbors_by_iface_[iface]->remote_id();
    ManaMessageProtobuf buff;
    // set the sender id
    buff.set_sender(id_);
    // fill in the rest of the message parts
    try {
    	to_protobuf(dynamic_cast<const ManaMessage&>(msg), buff);
    } catch(const exception& e) {
    	// ignore the message
    	FILE_LOG(logWARNING) << "Broker::handle_match: static_cast from siena::message& to ManaMessageProtobuf& failed.";
    	return true;
    }
    // FIXME: i've not yet found a way to remove a subscription from
    // an interface. So it can happen that a session is removed but
    // the subscription still exists in the forwarding table, hence
    // we need the check. Assertion is disabled for now ...
    //assert(is_in_container(neighbors_by_iface_, iface));
    // FIXME: we need read/write lock here
    if(is_in_container(neighbors_by_iface_, iface) == false)
        return true;
    neighbors_by_iface_[iface]->send(buff);
    //
    byte data[MAX_MSG_SIZE];
    assert(buff.SerializeToArray(data, MAX_MSG_SIZE));
    return true;
}

const string& Broker::id() const {
	return id_;
}

void Broker::handle_session_termination(Session<Broker>& s) {
	FILE_LOG(logDEBUG2) << "Broker::handle_session_termination: Session " << s.remote_id() << " terminated.";
    // FIXME: Locking needed
    iface_no_generator_.return_number(s.iface());
    neighbors_by_iface_.erase(s.iface());
    neighbors_by_id_.erase(s.remote_id());
    // and of course we need to find a way to remove the predicate from the
    // table and then return the interface number back to the pool. (disabled for now)
}

void Broker::send_error() {}

} /* namespace mana */
