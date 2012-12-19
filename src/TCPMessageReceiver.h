/**
 * @file TCPMessageReceiver.h
 * @brief A TCP message receiver.
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

#ifndef TCPMESSAGERECEIVER_H_
#define TCPMESSAGERECEIVER_H_

#include <vector>
#include <functional>
#include <memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "MessageReceiver.h"
#include "string"
#include "TCPNetworkConnector.h"
#include "ManaMessage.pb.h"
#include "TCPMessageReceiver.h"
#include "common.h"


using namespace std;

namespace mana {

template <class T>
class TCPMessageReceiver: public MessageReceiver<T> {
public:

/**
* @brief Constructor
* @param srv An instance of boost::asio::io_service;
* @param client An object the implements a handlemessage method to receive messages
* @param port Server's TCP port
* @param addr The local address of the server
*/
TCPMessageReceiver(boost::asio::io_service& srv, T& client,
		const URL& url) : MessageReceiver<T>(srv, client, url) {
	this->connection_type_ = mana::tcp;
}

virtual ~TCPMessageReceiver() {
	try {
		stop();
	} catch(exception& e) {/* ignore errors */}
}

void start() {
    if(this->flag_runing_)
            return;
    try {
        this->acceptor_ptr_ = make_shared<boost::asio::ip::tcp::acceptor>(this->io_service_);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), this->url_.port());
        this->acceptor_ptr_->open(endpoint.protocol());
        this->acceptor_ptr_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        this->acceptor_ptr_->bind(endpoint);
        this->acceptor_ptr_->listen();
        this->flag_runing_ = true;
        begin_accept();
    } catch(exception& e) {
        log_warn("TCPMessageReceiver<T>::start: error listening to socket on " << this->url_.url());
    }
}

void stop() {
	this->flag_runing_ = false;
	// cancel all ongoing i/o operations
	acceptor_ptr_->cancel();
	// stop listening on the socket
	acceptor_ptr_->close();
}

void begin_accept() {
    try {
            next_connection_socket_ = make_shared<boost::asio::ip::tcp::socket>(this->io_service_);
            acceptor_ptr_->async_accept(*next_connection_socket_, boost::bind(&TCPMessageReceiver<T>::accept_handler,
                            this, boost::asio::placeholders::error));
    } catch(exception& e) {
            log_warn("TCPMessageReceiver::begin_accept: an error occurred while accepting a new connection: " <<
                            e.what());
    }
}

void accept_handler(const boost::system::error_code& ec) {
    if(!ec && ec.value() != 0) {
            log_err("error: " << ec.message());
    }
    log_debug("Accepted connection from "  << next_connection_socket_->remote_endpoint().address().to_string());
    //once the connection is created we move the pointer
    shared_ptr<NetworkConnector<T>> c = make_shared<TCPNetworkConnector<T>>(std::move(next_connection_socket_),
    		this->client_, this->url_);
    // FIXME: at some point (which ideally must be after the connection termination) we must remove the pointer from the
    // vector otherwise we'll have some sort of memory leak, at a very minimum.
    this->connections_.push_back(c);
    this->client_.handle_connect(c);
    begin_accept();
}

private:
    // properties
    shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_ptr_;
    shared_ptr<boost::asio::ip::tcp::socket> next_connection_socket_;
    vector<shared_ptr<NetworkConnector<T>>> connections_;
};

} /* namespace mana */
#endif /* TCPMESSAGERECEIVER_H_ */
