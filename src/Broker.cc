/*
 * @file Broker.cc
 * Broker implementation
 *
 * @brief This class implements a messagin broker.
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
#include <boost/algorithm/string.hpp>
#include "ManaException.h"
#include "MessageReceiver.h"
#include "TCPMessageReceiver.h"
#include "UDPMessageReceiver.h"
#include "ManaFwdTypes.h"
#include "Session.h"
#include "Broker.h"
#include "TaskScheduler.h"
#include "common.h"

using namespace std;

namespace mana {


Broker::Broker(const string& id) : id_(id), num_of_threads_(DEFAULT_NUM_OF_BROKER_THREADS),
    task_scheduler_(io_service_) {
    message_match_handler_ = new BrokerMatchMessageHandler(this);
}

Broker::~Broker() {
    delete message_match_handler_;
}

void Broker::add_transport(string url) {
    vector<string> tokens;
    boost::split(tokens, url, boost::is_any_of(":"));
    string& addr = tokens[1];
    int port = 0;
    try {
            port = stoi(tokens[2]);
    } catch(exception& e) {
            throw ManaException("Could not connect: invalid port number: " + tokens[2]);
    }
    // convert to lowercase e.g., TCP -> tcp
    std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(), ::tolower);
    if(tokens[0] == "tcp") {
        //auto h = std::bind(&Broker::receive_handler, this, std::placeholders::_1, std::placeholders::_2);
        //function<void(shared_ptr<NetworkConnector>)> h_c = std::bind(&Broker::connect_handler, this, std::placeholders::_1);
        auto mr = make_shared<TCPMessageReceiver<Broker> >(io_service_, *this, port, addr);
        message_receivers.push_back(mr);
    } else {
        log_err("Malformed URL or method not supported:" << url);
        exit(-1);
    }
}

void Broker::start() {
    log_info("Starting broker...");
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
    	log_err("Broker::start: No active transport. Terminating.\n");
    	exit(-1);
    }
    try {
        // all threads except one get detached
        for (unsigned int i = 0; i < num_of_threads_ - 1; i++)
            std::thread([&](){io_service_.run();}).detach();
        // the last thread does not detach so we block here until the broker
        // is shutdown
        io_service_.run();
    } catch (exception& e) {
        log_err("An error happened in the broker execution: " << e.what());
    }
}

void Broker::shutdown() {
    log_info("Broker is shutting down...");
	io_service_.stop();
    log_info("done." << endl);
}

boost::asio::io_service& Broker::io_service() {
   return io_service_;
}

void Broker::handle_sub(NetworkConnector<Broker>& nc, ManaMessage& buff) {
    mana_filter* fltr = new mana_filter();
    to_mana_filter(buff, *fltr);
    //TODO: one predicate per filter is not the right way...
    mana_predicate pred;
    pred.add(fltr);
    // if the subscribing node is already in the list of neighbor
    // nodes then use the entry that was already created for this
    // neighbor to obtain interface number, etc. Otherwise create
    // a new entry for this subscriber.
    siena::if_t  if_no = 0;
    if(is_in_container(neighbors_by_id_, buff.sender())) {
        if_no =  neighbors_by_id_[buff.sender()]->iface();
    } else {
        if_no = iface_no_generator_.borrow_number();
        log_info(" iface no: " << if_no);
        auto tmp = make_shared<Session<Broker>>(*this, &nc, buff.sender(), if_no);
        neighbors_by_id_[buff.sender()] = tmp;
        neighbors_by_iface_[if_no] = std::move(tmp);
    }
    {// lock the forwarding table before inserting filters into it
        lock_guard<FwdTableWrapper> lock(fwd_table_wrapper_);
        fwd_table_wrapper_.fwd_table().ifconfig(if_no, pred);
        fwd_table_wrapper_.fwd_table().consolidate();
    }
}

void Broker::handle_not(ManaMessage& buff) {
    mana_message msg;
    to_mana_message(buff, msg);
    //FIXME:  this has to turn to read/write locking
    lock_guard<FwdTableWrapper> lock(fwd_table_wrapper_);
    fwd_table_wrapper_.const_fwd_table().match(msg, *message_match_handler_);
}

void Broker::handle_heartbeat(ManaMessage& buff) {
    if(is_in_container(neighbors_by_id_, buff.sender())) {
    	neighbors_by_id_[buff.sender()]->update_hb_reception_ts();
    }
}

/**
 * Brief: With TCP/KA transport this callback is called by the main acceptor.
 *
 * With TCP/KA transport this callback is called by the main acceptor. This
 * enables the Broker to memorize the connection for later use.
 */
void Broker::handle_connect(shared_ptr<NetworkConnector<Broker>>& c) {
    //TODO save a pointer to c in a new session ....
}

void Broker::handle_message(NetworkConnector<Broker>& nc, ManaMessage& msg) {
        switch(msg.type()) {
        case ManaMessage_message_type_t_SUB:
            handle_sub(nc, msg);
            break;
        case ManaMessage_message_type_t_NOT:
            handle_not(msg);
            break;
        case ManaMessage_message_type_t_HEARTBEAT:
        	handle_heartbeat(msg);
        	break;
        default: // other types ...
        	log_warn("Broker::handle_message: message of invalid typed was received.")
            break;
        }
        msg.Clear();
}

bool Broker::handle_match(siena::if_t iface, const siena::message& msg) {
    log_debug("Broker::handle_match(): match for client " << neighbors_by_iface_[iface]->id());
    ManaMessage buff;
    // set the sender id
    buff.set_sender(id_);
    // fill in the rest of the message parts
    try {
    	to_protobuf(dynamic_cast<const mana_message&>(msg), buff);
    } catch(exception& e) {
    	// ignore the message
    	log_warn("Broker::handle_match: static_cast from siena::message& to ManaMessage& failed.");
    	return true;
    }
    // FIXME: i've not yet found a way to remove a subscription from
    // an interface. So it can happen that a session is removed but
    // the subscription still exists in the forwarding table, hence
    // we need the ckeck. Assertion is disabled for now ...
    //assert(is_in_container(neighbors_by_iface_, iface));
    // FIXME: we need read/write lock here
    if(is_in_container(neighbors_by_iface_, iface) == false)
        return true;
    neighbors_by_iface_[iface]->net_connector()->send(buff);
    //
    unsigned char data[MAX_MSG_SIZE];
    assert(buff.SerializeToArray(data, MAX_MSG_SIZE));
    return true;
}

void Broker::handle_session_termination(Session<Broker>& s) {
    log_info("Broker::handle_session_termination: Session " << s.id() << " terminated.");
    // FIXME: Locking needed
    //iface_no_generator_.return_number(s.iface());
    neighbors_by_iface_.erase(s.iface());
    neighbors_by_id_.erase(s.id());
    // and of course we need to find a way to remnove the predicate from the
    // table and then return the interface number back to the pool. (disabled for now)
}

} /* namespace mana */
