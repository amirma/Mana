/*
 * TCPMessageReceiver.cpp
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <memory>
#include "TCPMessageReceiver.h"
#include "common.h"

using namespace std;

namespace mana {

TCPMessageReceiver::TCPMessageReceiver(boost::asio::io_service& srv,
		const int port, const string& addr, const std::function<void(NetworkConnector*, ManaMessage&)>& hndlr, 
        const std::function<void(shared_ptr<NetworkConnector>)>& c_hndlr):
		MessageReceiver(srv, hndlr), connect_handler_(c_hndlr) {
	connection_type_ = mana::tcp;
	port_ = port;
	address_ = addr;
}

TCPMessageReceiver::~TCPMessageReceiver() {
}

void TCPMessageReceiver::start() {
	if(flag_runing_)
		return;
	try {
		acceptor_ptr_ = make_shared<boost::asio::ip::tcp::acceptor>(io_service_);
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port_);
		acceptor_ptr_->open(endpoint.protocol());
		acceptor_ptr_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_ptr_->bind(endpoint);
		acceptor_ptr_->listen();
		flag_runing_ = true;
		begin_accept();
	} catch(exception& e) {
		log_warn("\nTCPMessageReceiver::start: error listening to socket on tcp://" <<
				address_ << ":"  << port_ << " ");
	}
}

void TCPMessageReceiver::stop() {
	flag_runing_ = false;
}

void TCPMessageReceiver::begin_accept() {
	try {
		next_connection_socket_ = make_shared<boost::asio::ip::tcp::socket>(io_service_);
		acceptor_ptr_->async_accept(*next_connection_socket_, boost::bind(&TCPMessageReceiver::accept_handler,
				this, boost::asio::placeholders::error));
	} catch(exception& e) {
		log_warn("\nTCPMessageReceiver::begin_accept: an error occured while accepting a new connection: " <<
				e.what());
	}
}

void TCPMessageReceiver::accept_handler(const boost::system::error_code& ec) {

	if(!ec && ec.value() != 0) {
		log_err("error: " << ec.message());
	}

	log_debug("\nAccepted connection from "  << next_connection_socket_->remote_endpoint().address().to_string());
	//once the connection in created we move the pointer
	auto c = make_shared<TCPNetworkConnector>(next_connection_socket_, receive_handler_);
	remote_endpoints.push_back(std::move(next_connection_socket_));
    if(connect_handler_ != nullptr)
        connect_handler_(c);
	tcp_connections.push_back(std::move(c));
	begin_accept();

}

} /* namespace mana */
