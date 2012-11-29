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

#include "Broker.h"
#include "common.h"
#include <memory>
#include <assert.h>
#include <thread>
#include "ManaException.h"
#include <boost/algorithm/string.hpp>
#include "MessageReceiver.h"
#include "TCPMessageReceiver.h"
#include "UDPMessageReceiver.h"
#include "ManaFwdTypes.h"

using namespace std;

namespace mana {


Broker::Broker(const string& id) : id_(id), num_of_threads_(DEFAULT_NUM_OF_THREADS), task_scheduler_(io_service_) {
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
        auto h = std::bind(&Broker::receive_handler, this, std::placeholders::_1, std::placeholders::_2);
        function<void(shared_ptr<NetworkConnector>)> h_c = std::bind(&Broker::connect_handler, this, std::placeholders::_1);
        auto mr = make_shared<TCPMessageReceiver>(io_service_ ,port, addr, h, h_c);
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
    	log_err("\nBroker::start: No active transport. Terminating.\n");
    	exit(-1);
    }
    // Start the periodical neighbor control and heartbeat task
    auto t = std::bind(&Broker::check_neighbors_and_send_hb, this);
    task_scheduler_.schedule_at_periods(std::move(t), DEFAULT_HEARTBEAT_INTERVAL_SECONDS, TaskScheduler::second);
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

void Broker::handle_message(ManaMessage& msg) {
   log_debug("\nbroker received message from sender " << msg.sender());
}

void Broker::handle_sub(NetworkConnector* nc, ManaMessage& buff) {
    mana_filter* fltr = new mana_filter();
    to_mana_filter(buff, *fltr);
    //TODO: one predicate per filter is not the right way...
    mana_predicate pred;
    pred.add(fltr);
    // if the subscribing node is already in the list of neighbor
    // nodes then use the entry that was already created for this
    // neighbor to obtain interface number, etc. Otherwise create
    // a new entry for this subscriber.
    siena::if_t  if_ = 0;
    if(is_in_container(neighbors_by_id_, buff.sender())) {
        if_ =  neighbors_by_id_[buff.sender()]->iface_;
    } else {
        if_ = iface_no_generator_.borrow_number();
        auto tmp = make_shared<NeighborNode>(nc, buff.sender(), if_);
        tmp->last_hb_reception_ts_ = std::chrono::system_clock::now();
        neighbors_by_id_[buff.sender()] = tmp;
        neighbors_by_iface_[if_] = std::move(tmp);
    }
    {// lock the forwarding table before inserting filters into it
        lock_guard<FwdTableWrapper> lock(fwd_table_wrapper_);
        fwd_table_wrapper_.fwd_table().ifconfig(if_, pred);
        fwd_table_wrapper_.fwd_table().consolidate();
    }
}

void Broker::handle_not(ManaMessage& buff) {
	mana_message msg;
	to_mana_message(buff, msg);
	// FIXME : do we need locking here ?
	fwd_table_wrapper_.const_fwd_table().match(msg, *message_match_handler_);
}

void Broker::handle_heartbeat(ManaMessage& buff) {
	if(is_in_container(neighbors_by_id_, buff.sender())) {
		neighbors_by_id_[buff.sender()]->last_hb_reception_ts_ = std::chrono::system_clock::now();
	}
}

/**
 * Brief: With TCP/KA transport this callback is called by the main acceptor.
 *
 * With TCP/KA transport this callback is called by the main acceptor. This
 * enables the Broker to memorize the connection for later use.
 */
void Broker::connect_handler(shared_ptr<NetworkConnector> connector) {

}

void Broker::receive_handler(NetworkConnector* nc, ManaMessage& msg) {
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
            handle_message(msg);
        }
        msg.Clear();
}

bool Broker::handle_match(siena::if_t iface, const siena::message& msg) {
        log_debug("\nBroker::handle_match(): match for client " << neighbors_by_iface_[iface]->id_);
        ManaMessage buff;
        // set the sender id
        buff.set_sender(id_);
        // fill in the rest of the message parts
        to_protobuf(dynamic_cast<const mana_message&>(msg), buff);
        assert(is_in_container(neighbors_by_iface_, iface));
        neighbors_by_iface_[iface]->net_connector_->send(buff);
        //
        unsigned char data[MAX_MSG_SIZE];
        assert(buff.SerializeToArray(data, MAX_MSG_SIZE));
        return true;
}

void Broker::check_neighbors_and_send_hb() {
    // FIXME: we need R/W locking here ...
    log_info("\nBroker::check_neighbors_and_send_hb: checking neighbors...");
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    // go through the list of neighbors and remove all those from which no heartbeat
    // was received during the last DEFAULT_HEARTBEAT_INTERVAL_SECONDS
    map<string, shared_ptr<NeighborNode>>::iterator it = neighbors_by_id_.begin();
    for(; it != neighbors_by_id_.end(); ++it) {
        if(now - it->second->last_hb_reception_ts_ > std::chrono::seconds(DEFAULT_HEARTBEAT_INTERVAL_SECONDS))  {
        	log_info("\nBroker::check_neighbor_and_send_hb: removing neighbor " << it->second->id_);
            it->second->net_connector_->disconnect();
            neighbors_by_iface_.erase(it->second->iface_);
            neighbors_by_id_.erase(it);
            // TODO: erase the guy from the fwd table.
        }
    }
    // send heartbeat messages to the neighbors
//    for(auto& neighbor : neighbors_by_id_) {

 //   }
}

} /* namespace mana */
