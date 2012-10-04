/*
 * TCPNetworkConnector.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "TCPNetworkConnector.h"
#include "boost/bind.hpp"
#include <boost/algorithm/string.hpp>
#include "exception"
#include "SienaPlusException.h"

namespace sienaplus {

/*
 * Construct an instance of this class with the given io_service.
 * This constructor is used when we need to manually create a connection.
 */
TCPNetworkConnector::TCPNetworkConnector(boost::asio::io_service& srv,
		const std::function<void(NetworkConnector*, const char*, int)>& hndlr) : NetworkConnector(srv, hndlr) {
	socket_ = make_shared<boost::asio::ip::tcp::socket>(io_service_);
}

/*
 * Construct an instance of this class with the given socket.
 * This constructor is usually used when we have obtained a socket from an
 * acceptor that accepts a net connection. Note that the passed socket (skt)
 * has to outlive the instance of this class.
 */
TCPNetworkConnector::TCPNetworkConnector(shared_ptr<boost::asio::ip::tcp::socket>& skt,
	const std::function<void(NetworkConnector*, const char*, int size)>& hndlr) :
	socket_(skt), NetworkConnector(skt->get_io_service(), hndlr) {
	if(socket_->is_open()) {
		flag_is_connected = true;
		start_read();
	}
}

TCPNetworkConnector::~TCPNetworkConnector() {
}

void TCPNetworkConnector::async_connect(const string& url) {
	if(flag_is_connected)
		return;
	vector<string> tokens;
	boost::split(tokens, url, boost::is_any_of(":"));
	int port = 0;
	try {
		port = stoi(tokens[2]);
	} catch(exception& e) {
		throw SienaPlusException("Could not connect: invalid port number: " + tokens[2]);
	}
	async_connect(tokens[1], port);
}

void TCPNetworkConnector::async_connect(const string& addr, int prt) {
	if(flag_is_connected)
		return;
	try {
		boost::asio::ip::tcp::endpoint endpnt(boost::asio::ip::address::from_string(addr), prt);
		socket_->async_connect(endpnt, boost::bind(&TCPNetworkConnector::connect_handler, this, boost::asio::placeholders::error));
	} catch(exception& e) {
		flag_is_connected = false;
		log_info("count not connect");
	}
}

void TCPNetworkConnector::write_handler(const boost::system::error_code& error, std::size_t bytes_transferred) {
    log_debug("TCPNetworkConnector::write_handler(): wrote " << bytes_transferred << " bytes.");
}

void TCPNetworkConnector::connect_handler(const boost::system::error_code& ec) {
	if(!ec && ec.value() != 0) {
	    log_info("could not connect...");
        return;
	}
	//socket_
	flag_is_connected = true;
	start_read();
}

void TCPNetworkConnector::start_read() {
    // TODO : allocate a buffer and remove it later...this is not efficient for
    // sure !
	//char* read_buffer_ = new char[MAX_MSG_SIZE];
    socket_->async_read_some(boost::asio::buffer(read_buffer_, MAX_MSG_SIZE), boost::bind(&TCPNetworkConnector::read_handler, this,
    	boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    //async_read(*socket_, boost::asio::buffer(read_buffer_), boost::bind(&TCPNetworkConnector::read_handler, this,
	//	boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

}

void TCPNetworkConnector::read_handler(const boost::system::error_code& ec, std::size_t bytes_num) {
	if(!ec && ec.value() != 0) {
		log_err("error reading:" << ec.message());
		return;
	}
	// TODO: i'm using the number of bytes as a hint that the connection 
    // terminated. I'm not sure this is a good way though. For some 
    // reason socket.is_open() does not do it's job...
	if(bytes_num == 0) {
        flag_is_connected = false;
        return;
	}
    log_debug("TCPNetworkConnector::read_handler(): read " << bytes_num << " bytes.");
	assert(receive_handler != NULL);
	receive_handler(this, read_buffer_.data(), bytes_num);
    // FIXME: is this needed ?
    if(is_connected())
        start_read();
}

void TCPNetworkConnector::send(const char* data, size_t length) {
	if(!is_connected()) {
		log_err("TCPNetworkConnector::send(): socket is not connected. not sending.");
		return;
	}
    boost::asio::async_write(*socket_, boost::asio::buffer(data, length), 
            boost::bind(&TCPNetworkConnector::write_handler, this, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    log_debug("TCPNetworkConnector::send(): sent " << length << " bytes.");
	//socket_->write_some(boost::asio::buffer(data, length));
}

void TCPNetworkConnector::send(const string& str) {
	if(!is_connected()) {
		log_err("TCPNetworkConnector::send(): socket is not connected. not sending.");
		return;
	}

	//socket_->write_some(boost::asio::buffer(str.c_str(), str.length()));
    boost::asio::async_write(*socket_, boost::asio::buffer(str.c_str(), str.length()), 
            boost::bind(&TCPNetworkConnector::write_handler, this, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    
}

bool TCPNetworkConnector::connect(const string& url) {
	if(is_connected()) {
		return true;
	}
	vector<string> tokens;
	boost::split(tokens, url, boost::is_any_of(":"));
	int port = 0;
	try {
		port = stoi(tokens[2]);
	} catch(exception& e) {
		throw SienaPlusException("Could not connect: invalid port number: " + tokens[2]);
	}
	connect(tokens[1], port);
}

bool TCPNetworkConnector::connect(const string& addr, int prt) {
	if(is_connected()) {
		return true;
	}

	try {
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(addr), prt);
		socket_->connect(endpoint);
	} catch(boost::system::system_error& e) {
		throw SienaPlusException("Error connecting to endpoint.");
	} catch(exception& e) {
		throw SienaPlusException("Error connecting to endpoint.");
	}
	//
	flag_is_connected = true;
	log_debug("TCPConnector is connected.");
	start_read();
}

void TCPNetworkConnector::disconnect() {
    socket_->close();
}

bool TCPNetworkConnector::is_connected() {
	return socket_->is_open();
}

} /* namespace sienaplus */
