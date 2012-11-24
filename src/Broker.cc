/**
 * @file Broker.cpp
 * Broker implementation
 *
 * @brief Can use "brief" tag to explicitly generate comments for file documentation.
 *
 * @author Amir Malekpour
 * @version 0.1
 */

#include "Broker.h"
#include "common.h"
#include <memory>
#include <assert.h>
#include <thread>
#include "SienaPlusException.h"
#include <boost/algorithm/string.hpp>
#include "MessageReceiver.h"
#include "TCPMessageReceiver.h"
#include "UDPMessageReceiver.h"
#include "ManaFwdTypes.h"


using namespace std;

namespace sienaplus {


Broker::Broker(const string& id) : id_(id) {
    message_match_handler_ = new BrokerMatchMessageHandler(this);
    num_of_threads_ = DEFAULT_NUM_OF_THREADS;
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
		throw SienaPlusException("Could not connect: invalid port number: " + tokens[2]);
	}

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
	for(auto tr : message_receivers)
		tr->start();
    try {
        for (unsigned int i = 0; i < num_of_threads_ - 1; i++)
            std::thread([&](){io_service_.run();}).detach();
        io_service_.run();
	} catch (SienaPlusException& e) {
	    log_err("An error happended in the broker execution: " << e.what());
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

void Broker::handle_message(SienaPlusMessage& msg) {
   log_debug("\nbroker received message from sender " << msg.sender());
}

void Broker::handle_sub(NetworkConnector* nc, SienaPlusMessage& buff) {
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
        neighbors_by_id_[buff.sender()] = tmp;
        neighbors_by_iface_[if_] = std::move(tmp);
    }
    {// lock the forwarding table before inserting filters into it
        lock_guard<FwdTableWrapper> lock(fwd_table_wrapper_);
        fwd_table_wrapper_.fwd_table().ifconfig(if_, pred);
        fwd_table_wrapper_.fwd_table().consolidate();
    }
}

void Broker::handle_not(SienaPlusMessage& buff) {
  mana_message msg;
  to_mana_message(buff, msg);
  // FIXME : do we need locking here ?
  fwd_table_wrapper_.const_fwd_table().match(msg, *message_match_handler_);
}

/*
 * Brief: With TCP/KA transport this callback is called by the main acceptor.
 *
 * With TCP/KA transport this callback is called by the main acceptor. This
 * enables the Broker to memorize the connection for later use.
 */
void Broker::connect_handler(shared_ptr<NetworkConnector> connector) {
}

void Broker::receive_handler(NetworkConnector* nc, SienaPlusMessage& msg) {
        switch(msg.type()) {
        case SienaPlusMessage_message_type_t_SUB:
            handle_sub(nc, msg);
            break;
        case SienaPlusMessage_message_type_t_NOT:
            handle_not(msg);
            break;
        default: // other types ...
            handle_message(msg);
        }
        msg.Clear();
}

bool Broker::handle_match(siena::if_t iface, const siena::message& msg) {
        log_debug("\nBroker::handle_match(): match for client " << neighbors_by_iface_[iface]->id_);
        SienaPlusMessage buff;
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
} /* namespace sienaplus */
