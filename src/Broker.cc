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
#include "SienaPlusException.h"
#include <boost/algorithm/string.hpp>
#include "MessageReceiver.h"
#include "TCPMessageReceiver.h"
#include "UDPMessageReceiver.h"
#include "sff.bzr/simple_fwd_types.h"


using namespace std;

namespace sienaplus {


Broker::Broker(const string& id) : id_(id) {
    message_match_handler_ = new BrokerMatchMessageHandler(this);
    // TODO: the broker id has to be set at command line...
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
		//std::shared_ptr<MessageReceiver> mr(new TCPMessageReceiver(io_service_ ,port, addr, h));
		std::shared_ptr<MessageReceiver> mr(new TCPMessageReceiver(io_service_ ,port, addr, [&](const char* data,
				int size){receive_handler(data, size);}, h_c));
		message_receivers.push_back(mr);

	} else {
		cout << "Malformed URL or method not supported:" << url << endl;
		exit(-1);
	}
}

void Broker::start() {
	for(auto tr : message_receivers)
		tr->start();
	for (;;) {
	  try {
	    io_service_.run();
	    break; // run() exited normally
	  } catch (SienaPlusException& e) {
	    cout << "An error happended in the broker execution: " << e.what();
	  }
	}
}

void Broker::shutdown() {
    cout << endl << "Broker is shutting down...";
	io_service_.stop();
    cout << "done." << endl;
}

boost::asio::io_service& Broker::io_service() {
	return io_service_;
}

void Broker::handle_message(SienaPlusMessage& msg) {
	cout << "broker received message from sender " << msg.sender();
}

void Broker::handle_sub(SienaPlusMessage& buff) {
    simple_filter* fltr = new simple_filter();
    to_simple_filter(buff, *fltr);
    //TODO: one predicate per filter is not the right way...
    simple_predicate pred;
    pred.add(fltr);

    siena::if_t  if_ = 0;
    //if(is_in_container(neighbors_by_id_, buff.sender())) {
    if(neighbors_by_id_.find(buff.sender())!=neighbors_by_id_.end()) {
        if_ =  neighbors_by_id_[buff.sender()]->iface_;
    } else {
        if_ = iface_no_generator_.borrow_number();
        auto tmp = make_shared<NeighborNode>(buff.sender(), if_);
        neighbors_by_id_[buff.sender()] = tmp;
        neighbors_by_iface_[if_] = std::move(tmp);
    }

    // FIXME: we definately need locking here ... 
    fwd_table_.ifconfig(if_, pred);
    fwd_table_.consolidate();
}

void Broker::handle_not(SienaPlusMessage& buff) {
  simple_message msg;
  to_simple_message(buff, msg);
  fwd_table_.match(msg, *message_match_handler_);
}

/*
 * Brief: With TCP/KA transport this callback is called by the main acceptor.
 *
 * With TCP/KA transport this callback is called by the main acceptor. This
 * enables the Broker to memorize the connection for later use.
 */
void Broker::connect_handler(shared_ptr<NetworkConnector> connector) {
    //TODO: with KA we must first check to see if this client is 
    //already in the list of neighbors
    //iface for this neighbor
    //unsigned int iface = iface_no_generator.borrow();
    //neighbors_[
}

void Broker::receive_handler(const char* data, int size) {
	SienaPlusMessage msg;
	if(!msg.ParsePartialFromArray(data, size)) {
		cout << endl << "Broker::receive_handler: unable to deserialize message.";
		return;
	}
    switch(msg.type()) {
        case SienaPlusMessage_message_type_t_SUB:
            handle_sub(msg);
            break;
        case SienaPlusMessage_message_type_t_NOT:
            handle_not(msg);
            break;
        //TODO : other types ...
        default:
	        handle_message(msg);
    }
}

bool Broker::handle_match(siena::if_t iface, const siena::message& msg) {
        cout << endl << "match for client " << neighbors_by_iface_[iface]->id_;
        return true;
    }
} /* namespace sienaplus */
