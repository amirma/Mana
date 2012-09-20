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
#include "ConnectionType.h"

using namespace std;

namespace sienaplus {

TCPMessageReceiver::TCPMessageReceiver(boost::asio::io_service& srv,
		const int port, const string& addr, std::function<void(const char*, int)> hndlr):
		MessageReceiver(srv, hndlr) {
	connection_type_ = sienaplus::tcp;
	port_ = port;
	address_ = addr;
	flag_runing_ = false;
}

TCPMessageReceiver::~TCPMessageReceiver() {
}

void TCPMessageReceiver::start() {
	if(flag_runing_)
		return;
	//acceptor_ptr_ = shared_ptr<boost::asio::ip::tcp::acceptor>(new boost::asio::ip::tcp::acceptor(io_service_));
	acceptor_ptr_ = make_shared<boost::asio::ip::tcp::acceptor>(io_service_);
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port_);
	acceptor_ptr_->open(endpoint.protocol());
	acceptor_ptr_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_ptr_->bind(endpoint);
	acceptor_ptr_->listen();
	flag_runing_ = true;
	begin_accept();
}

void TCPMessageReceiver::stop() {
	flag_runing_ = false;
}

void TCPMessageReceiver::begin_accept() {
	next_connection_socket_ = make_shared<boost::asio::ip::tcp::socket>(io_service_);
	acceptor_ptr_->async_accept(*next_connection_socket_, boost::bind(&TCPMessageReceiver::accept_handler,
	this, boost::asio::placeholders::error));
}

void TCPMessageReceiver::accept_handler(const boost::system::error_code& ec) {

	if(!ec && ec.value() != 0) {
		cout << "error: " << ec.message();
	}

	cout << endl << "Accepted connection from "  << next_connection_socket_->remote_endpoint().address().to_string() << endl;
	//once the connection in created we move the pointer
	auto c = make_shared<TCPNetworkConnector>(next_connection_socket_, receive_handler_);
	remote_endpoints.push_back(std::move(next_connection_socket_));
	tcp_connections.push_back(std::move(c));
	begin_accept();
}

} /* namespace sienaplus */
