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


Broker::Broker() {
}

Broker::~Broker() {
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
	for (;;)
	{
	  try
	  {
	    io_service_.run();
	    break; // run() exited normally
	  }
	  catch (SienaPlusException& e)
	  {
	    // Deal with exception as appropriate.
	  }
	}
}

void Broker::shutdown() {
	io_service_.stop();
}

boost::asio::io_service& Broker::io_service() {
	return io_service_;
}

void Broker::handle_message(SienaPlusMessage& msg) {
	cout << "broker received message from sender " << msg.sender();
}

void Broker::handle_sub(SienaPlusMessage& msg) {
    simple_filter* fltr = new simple_filter();
    cout << endl << "size: " << msg.subscription().constraints_size();
    for(int i = 0; i < msg.subscription().constraints_size(); i++) {
        string* st = new string(msg.subscription().constraints(i).name());
        // TODO: string_t
        siena::string_t name(st->c_str());
        siena::operator_id op_id = siena::operator_id(msg.subscription().constraints(i).op());
        switch(msg.subscription().constraints(i).value().type()) {
            case SienaPlusMessage_tag_type_t_STRING: {
                // TODO : I don't like the convesrion of string to siena string
                // ...
                string* st = new string(msg.subscription().constraints(i).value().string_value());
                // TODO: string_t again...
                simple_op_value* sopv = 
                    new simple_op_value(op_id, siena::string_t(st->c_str()));
                fltr->add(name, sopv);
                break;
            }
            case SienaPlusMessage_tag_type_t_INT: {
                simple_op_value* sopv = 
                    new simple_op_value(op_id, static_cast<siena::int_t>(msg.subscription().constraints(i).value().int_value()));
                fltr->add(name, sopv);
                break;
            }
            case SienaPlusMessage_tag_type_t_DOUBLE: {
                simple_op_value* sopv = 
                    new simple_op_value(op_id, static_cast<siena::double_t>(msg.subscription().constraints(i).value().double_value()));
                fltr->add(name, sopv);
                break;
            }
            case SienaPlusMessage_tag_type_t_BOOLEAN: {
                simple_op_value* sopv  = 
                    new simple_op_value(op_id, static_cast<siena::bool_t>(msg.subscription().constraints(i).value().bool_value()));
                fltr->add(name, sopv);
                break;
            }
            default:
                cout << endl << "Warining: Broker.cc::handle_sub(): ignoring unrecognized constraint type";
        }
    }

    //TODO: one predicate per filter is not the right way...
    simple_predicate pred;
    pred.add(fltr);
    fwd_table_.ifconfig(iface_no_generator_.borrow_number(), pred);
    fwd_table_.consolidate();
}

void Broker::handle_not(SienaPlusMessage& msg) {
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
	//cout << endl << "broker received: " << data;

	//cout << endl << "here 0";
	SienaPlusMessage msg;
	//cout << endl << "here 1";

	if(!msg.ParsePartialFromArray(data, size)) {
		cout << endl << "Broker::receive_handler: unable to deserialize message.";
		return;
	}

	cout << endl << "type: " <<  msg.type();
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

} /* namespace sienaplus */
