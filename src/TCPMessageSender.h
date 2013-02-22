/**
 * @file TCPMessageSender.h
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

#ifndef TCPMessageSender_H_
#define TCPMessageSender_H_

#include <memory.h>
#include <thread>
#include <boost/algorithm/string.hpp>
#include "MessageSender.h"
#include "boost/bind.hpp"
#include "exception"
#include "ManaException.h"
#include "Log.h"

namespace mana {

/**
 * TCPMessage sender sends messages on a TCP connection. Upon a send a TCP connection
 * is established and terminated after send is complete. For details on T see
 * @link MessageSender @endlink
 *
 * @see MessageSender
 * @see UDPMessageSender
 */
template <class T>
class TCPMessageSender: public MessageSender<T> {

public:
    TCPMessageSender(const TCPMessageSender& other) = delete; // delete copy ctor
    TCPMessageSender& operator=(const TCPMessageSender&) = delete; // delete assignment operator
/**
 * Construct an instance of this class with the given io_service.
 * This constructor is used when we need to manually create a connection.
 */
TCPMessageSender(boost::asio::io_service& srv, T& c, const URL& url) :
		MessageSender<T>(srv, c, url), flag_try_reconnect_(false) {
	socket_ = make_shared<boost::asio::ip::tcp::socket>(this->io_service_);
}

/*
 * Construct an instance of this class with the given socket.
 * This constructor is usually used when we have obtained a socket from an
 * acceptor that accepts a net connection. Note that the passed socket (skt)
 * has to outlive the instance of this class.
 */
TCPMessageSender(shared_ptr<boost::asio::ip::tcp::socket>&& skt,
	T& c, const URL& url) : MessageSender<T>(skt->get_io_service(), c, url),
			socket_(skt), flag_try_reconnect_(false) {
	if(socket_->is_open()) {
		this->flag_is_connected = true;
        set_socket_options();
		//start_read();
	}
}

virtual ~TCPMessageSender() {
}

void set_socket_options() {
#ifdef DISABLE_NAGLE_ALG
    //disable the Nagle's algorithm so that small-sized messages
    //will be pushed as soon as send is called
    boost::asio::ip::tcp::no_delay option_op(true);
    socket_->set_option(option_op);
#endif
}

/**
 * @brief Connect to the provided url
 */
virtual bool connect() {
	if(is_connected()) {
		return true;
	}
	try {
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(this->url_.address()), this->url_.port());
		socket_->connect(endpoint);
	} catch(boost::system::system_error& e) {
		throw ManaException("Error connecting to the endpoint.");
	} catch(exception& e) {
		throw ManaException("Error connecting to the endpoint.");
	}
	FILE_LOG(logDEBUG2)  << "TCPConnector is connected to " << this->url_.url();
	//start_read();
    return true;
}

/**
 * @brief Wait for the ongoing transmissions to finish and then
 * terminate the connection
 */
virtual void disconnect() {
    // TODO: check if there's an ongoing transmission...
    boost::system::error_code ignored_ec;
    socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_->close();
}

virtual bool is_connected() {
	return socket_->is_open();
}

// private methods
private:
virtual void async_connect(const string& addr, int prt) {
	if(this->is_connected())
		return;
	try {
		boost::asio::ip::tcp::endpoint endpnt(boost::asio::ip::address::from_string(addr), prt);
		socket_->async_connect(endpnt, boost::bind(&TCPMessageSender<T>::connect_handler, this, boost::asio::placeholders::error));
	} catch(exception& e) {
		FILE_LOG(logWARNING)  << "TCPMessageSender::async_connect(): count not connect";
	}
}

void connect_handler(const boost::system::error_code& ec) {
	if(!ec && ec.value() != 0) {
		FILE_LOG(logWARNING)  << "TCPMessageSender::connect_handler(): could not connect.";
        return;
	}
    set_socket_options();
	//start_read();
}
/*
void start_read() {
    socket_->async_read_some(boost::asio::buffer(this->read_buffer_, MAX_MSG_SIZE),
            this->read_hndlr_strand_.wrap(boost::bind(&TCPMessageSender<T>::read_handler, this,
    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}
*/

/**
 * This method has only internal purposes and is used to send a buffer of
 * a given size using the socket.
*/
virtual void send_buffer(const unsigned char* data, size_t length) override {
	assert(this->write_buff_item_qu_.try_lock() == false);
	if(!is_connected()) {
            try {
                connect(); // fixme: because connect is syncronized this whole method is synced. Must be fixed.
            } catch(exception& e) {
		FILE_LOG(logWARNING)  << "TCPMessageSender::send_buffer(): socket could not connected. not sending.";
		return;
            }
	}
    assert(data[0] == BUFF_SEPERATOR);
    assert(*((int*)(data + BUFF_SEPERATOR_LEN_BYTE)) + MSG_HEADER_SIZE ==  static_cast<long>(length));
    boost::asio::async_write(*socket_, boost::asio::buffer(data, length),
        this->write_hndlr_strand_.wrap(boost::bind(&TCPMessageSender<T>::write_handler,
        this, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)));
    FILE_LOG(logDEBUG3) << "TCPMessageSender::send_buffer(): sent " << length << " bytes.";
    //
    disconnect();
}

// properties
shared_ptr<boost::asio::ip::tcp::socket> socket_;
bool flag_try_reconnect_; // this flag specifies the behavior in case
	// of connection termination, whether a re-connection should be tried or not
};

} /* namespace mana */
#endif /* TCPMessageSender_H_ */
