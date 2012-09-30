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
		const std::function<void(const char*, int)>& hndlr) : NetworkConnector(srv, hndlr) {
	socket_ = make_shared<boost::asio::ip::tcp::socket>(io_service_);
}

/*
 * Construct an instance of this class with the given socket.
 * This constructor is usually used when we have obtained a socket from an
 * acceptor that accepts a net connection. Note that the passed socket (skt)
 * has to outlive the instance of this class.
 */
TCPNetworkConnector::TCPNetworkConnector(shared_ptr<boost::asio::ip::tcp::socket>& skt,
	const std::function<void(const char*, int size)>& hndlr) :
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
		cout << "count not connect";
	}
}

void TCPNetworkConnector::connect_handler(const boost::system::error_code& ec) {
	if(!ec && ec.value() != 0) {
		// error
	}
	//socket_
	flag_is_connected = true;
	start_read();
}

void TCPNetworkConnector::start_read() {
	socket_->async_read_some(boost::asio::buffer(read_buffer_), boost::bind(&TCPNetworkConnector::read_handler, this,
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void TCPNetworkConnector::read_handler(const boost::system::error_code& ec, std::size_t bytes_num) {
	if(!ec && ec.value() != 0) {
		cout << endl << "error reading:" << ec.message() << endl;
		return;
	}
	/*
	if(bytes_num != 0) {
		cout << endl << "read " << bytes_num << " bytes.";
		cout << endl << read_buffer_.data();
	}
	*/
	assert(receive_handler != NULL);
	receive_handler(read_buffer_.data(), bytes_num);
}

void TCPNetworkConnector::send(const char* data, int length) {
	if(!flag_is_connected) {
		cout << endl << "is not connected. not sending data.";
		return;
	}
	socket_->write_some(boost::asio::buffer(data, length));
}

void TCPNetworkConnector::send(const string& str) {
	if(!flag_is_connected) {
		cout << endl << "is not connected. not sending." << str;
		return;
	}
	socket_->write_some(boost::asio::buffer(str.c_str(), str.length()));
}

bool TCPNetworkConnector::connect(const string& url) {
	if(flag_is_connected) {
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
	if(flag_is_connected) {
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
	//cout << endl << "TCPConnector is connected.";
	start_read();
}

void TCPNetworkConnector::disconnect() {
}

bool TCPNetworkConnector::is_connected() {
	return socket_->is_open();
}

} /* namespace sienaplus */
