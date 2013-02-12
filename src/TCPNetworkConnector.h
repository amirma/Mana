/**
 * TCPNetworkConnector.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef TCPNetworkConnector_H_
#define TCPNetworkConnector_H_

#include <memory.h>
#include <thread>
#include <boost/algorithm/string.hpp>
#include "NetworkConnector.h"
#include "boost/bind.hpp"
#include "exception"
#include "ManaException.h"
#include "Log.h"

namespace mana {

template <class T>
class TCPNetworkConnector: public NetworkConnector<T> {

public:
    TCPNetworkConnector(const TCPNetworkConnector& other) = delete; // delete copy ctor
    TCPNetworkConnector& operator=(const TCPNetworkConnector&) = delete; // delete assignment operator
/**
 * Construct an instance of this class with the given io_service.
 * This constructor is used when we need to manually create a connection.
 */
TCPNetworkConnector(boost::asio::io_service& srv, T& c, const URL& url) :
		NetworkConnector<T>(srv, c, url), flag_try_reconnect_(false) {
	socket_ = make_shared<boost::asio::ip::tcp::socket>(this->io_service_);
}

/*
 * Construct an instance of this class with the given socket.
 * This constructor is usually used when we have obtained a socket from an
 * acceptor that accepts a net connection. Note that the passed socket (skt)
 * has to outlive the instance of this class.
 */
TCPNetworkConnector(shared_ptr<boost::asio::ip::tcp::socket>&& skt,
	T& c, const URL& url) : NetworkConnector<T>(skt->get_io_service(), c, url),
			socket_(skt), flag_try_reconnect_(false) {
	if(socket_->is_open()) {
		this->flag_is_connected = true;
        set_socket_options();
		//start_read();
	}
}

virtual ~TCPNetworkConnector() {
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
		socket_->async_connect(endpnt, boost::bind(&TCPNetworkConnector<T>::connect_handler, this, boost::asio::placeholders::error));
	} catch(exception& e) {
		FILE_LOG(logWARNING)  << "TCPNetworkConnector::async_connect(): count not connect";
	}
}

void connect_handler(const boost::system::error_code& ec) {
	if(!ec && ec.value() != 0) {
		FILE_LOG(logWARNING)  << "TCPNetworkConnector::connect_handler(): could not connect.";
        return;
	}
    set_socket_options();
	//start_read();
}
/*
void start_read() {
    socket_->async_read_some(boost::asio::buffer(this->read_buffer_, MAX_MSG_SIZE),
            this->read_hndlr_strand_.wrap(boost::bind(&TCPNetworkConnector<T>::read_handler, this,
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
		FILE_LOG(logWARNING)  << "TCPNetworkConnector::send_buffer(): socket is not connected. not sending.";
		return;
	}
    assert(data[0] == BUFF_SEPERATOR);
    assert(*((int*)(data + BUFF_SEPERATOR_LEN_BYTE)) + MSG_HEADER_SIZE ==  static_cast<long>(length));
    boost::asio::async_write(*socket_, boost::asio::buffer(data, length),
        this->write_hndlr_strand_.wrap(boost::bind(&TCPNetworkConnector<T>::write_handler,
        this, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)));
    FILE_LOG(logDEBUG3) << "TCPNetworkConnector::send_buffer(): sent " << length << " bytes.";
}

// properties
shared_ptr<boost::asio::ip::tcp::socket> socket_;
bool flag_try_reconnect_; // this flag specifies the behavior in case
	// of connection termination, whether a re-connection should be tried or not
};

} /* namespace mana */
#endif /* TCPNetworkConnector_H_ */
