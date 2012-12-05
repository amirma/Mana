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

//namespace mana {

template <class T>
TCPMessageReceiver<T>::TCPMessageReceiver(boost::asio::io_service& srv, T& client,
		const int port, const string& add) : MessageReceiver<T>(srv, client, port, add) {
	this->connection_type_ = mana::tcp;
}

template <class T>
TCPMessageReceiver<T>::~TCPMessageReceiver() {}

template <class T>
void TCPMessageReceiver<T>::start() {
	if(this->flag_runing_)
		return;
	try {
		this->acceptor_ptr_ = make_shared<boost::asio::ip::tcp::acceptor>(this->io_service_);
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), this->port_);
		this->acceptor_ptr_->open(endpoint.protocol());
		this->acceptor_ptr_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		this->acceptor_ptr_->bind(endpoint);
		this->acceptor_ptr_->listen();
		this->flag_runing_ = true;
		begin_accept();
	} catch(exception& e) {
		log_warn("\nTCPMessageReceiver<T>::start: error listening to socket on tcp://" <<
				this->address_ << ":"  << this->port_ << " ");
	}
}

template <class T>
void TCPMessageReceiver<T>::stop() {
	this->flag_runing_ = false;
}

template <class T>
void TCPMessageReceiver<T>::begin_accept() {
	try {
		next_connection_socket_ = make_shared<boost::asio::ip::tcp::socket>(this->io_service_);
		acceptor_ptr_->async_accept(*next_connection_socket_, boost::bind(&TCPMessageReceiver<T>::accept_handler,
				this, boost::asio::placeholders::error));
	} catch(exception& e) {
		log_warn("\nTCPMessageReceiver::begin_accept: an error occurred while accepting a new connection: " <<
				e.what());
	}
}

template <class T>
void TCPMessageReceiver<T>::accept_handler(const boost::system::error_code& ec) {
	if(!ec && ec.value() != 0) {
		log_err("error: " << ec.message());
	}
	log_debug("\nAccepted connection from "  << next_connection_socket_->remote_endpoint().address().to_string());
	//once the connection is created we move the pointer
	shared_ptr<NetworkConnector<T>> c = make_shared<TCPNetworkConnector<T>>(std::move(next_connection_socket_), this->client_);
        this->connections_.push_back(c);
        this->client_.handle_connect(c);
	begin_accept();
}

//} /* namespace mana */
