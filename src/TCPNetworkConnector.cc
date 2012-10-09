/*
 * TCPNetworkConnector.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "TCPNetworkConnector.h"
#include "boost/bind.hpp"
#include <boost/algorithm/string.hpp>
#include <thread>
#include "exception"
#include "SienaPlusException.h"

namespace sienaplus {

/*
 * Construct an instance of this class with the given io_service.
 * This constructor is used when we need to manually create a connection.
 */
TCPNetworkConnector::TCPNetworkConnector(boost::asio::io_service& srv,
		const std::function<void(NetworkConnector*, SienaPlusMessage&)>& hndlr) : 
    NetworkConnector(srv, hndlr) {
	socket_ = make_shared<boost::asio::ip::tcp::socket>(io_service_);
    read_hndlr_strand_ = make_shared<boost::asio::io_service::strand>(io_service_);
}

/*
 * Construct an instance of this class with the given socket.
 * This constructor is usually used when we have obtained a socket from an
 * acceptor that accepts a net connection. Note that the passed socket (skt)
 * has to outlive the instance of this class.
 */
TCPNetworkConnector::TCPNetworkConnector(shared_ptr<boost::asio::ip::tcp::socket>& skt,
	const std::function<void(NetworkConnector*, SienaPlusMessage&)>& hndlr) :
	socket_(skt), NetworkConnector(skt->get_io_service(), hndlr) {
    read_hndlr_strand_ = make_shared<boost::asio::io_service::strand>(io_service_);
	if(socket_->is_open()) {
		flag_is_connected = true;
        set_socket_options();
		start_read();
		//start_sync_read();
	}
}

TCPNetworkConnector::~TCPNetworkConnector() { 
}

void TCPNetworkConnector::set_socket_options() {
#ifdef DISABLE_NAGLE_ALG
    //disable the Nagle's algorithm so that small-sized messages 
    //will be pushed as soon as send is called
    boost::asio::ip::tcp::no_delay option_op(true);
    socket_->set_option(option_op);
#endif
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
    log_debug("\nTCPNetworkConnector::write_handler(): wrote " << bytes_transferred << " bytes.");
}

void TCPNetworkConnector::connect_handler(const boost::system::error_code& ec) {
	if(!ec && ec.value() != 0) {
	    log_info("could not connect...");
        return;
	}
    set_socket_options();
	flag_is_connected = true;
	start_read();
	//start_sync_read();
}

void TCPNetworkConnector::start_read() {
    // TODO : allocate a buffer and remove it later...this is not efficient for
    // sure !

    socket_->async_read_some(boost::asio::buffer(read_buffer_, MAX_MSG_SIZE), 
            read_hndlr_strand_->wrap(boost::bind(&TCPNetworkConnector::read_handler, this,
    	boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
    
    /*
    async_read(*socket_, boost::asio::buffer(read_buffer_), 
            read_hndlr_strand_->wrap(boost::bind(&TCPNetworkConnector::read_handler, this,
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
    */
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
        log_debug("\nTCPNetworkConnector::read_handler(): connection seems to be closed.");
        return;
	}
    log_debug("\nTCPNetworkConnector::read_handler(): read " << bytes_num << " bytes.");
	assert(receive_handler != NULL);

    SienaPlusMessage msg;
    message_stream_.consume(read_buffer_.data(), bytes_num);
    while(message_stream_.produce(msg))
	    receive_handler(this, msg);

    if(is_connected())
        start_read();
    //
	//receive_handler(this, tmp_buffer, bytes_num);
	//io_service_.post(bind(receive_handler, this, tmp_buffer, bytes_num));
}

void TCPNetworkConnector::start_sync_read() {
    std::thread([&](){
        for(;;) {
            try {
            size_t bytes_num = socket_->read_some(boost::asio::buffer(read_buffer_, MAX_MSG_SIZE));
            SienaPlusMessage msg;
            message_stream_.consume(read_buffer_.data(), bytes_num);
            while(message_stream_.produce(msg))
        	    receive_handler(this, msg);
            log_debug("\nTCPNetworkConnector::start_sync_read: read " << bytes_num << " bytes.");
            } catch(exception& e) {
                disconnect();        
            }
        }
    }).detach();
}

void TCPNetworkConnector::send(const void* data, size_t length) {
	if(!is_connected()) {
		log_err("TCPNetworkConnector::send(): socket is not connected. not sending.");
		return;
	}
    boost::asio::async_write(*socket_, boost::asio::buffer(data, length), 
            boost::bind(&TCPNetworkConnector::write_handler, this, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    log_debug("\nTCPNetworkConnector::send(): sent " << length << " bytes.");
	//socket_->write_some(boost::asio::buffer(data, length));
}

void TCPNetworkConnector::send(const string& str) {
    send(str.c_str(), str.length()); 
}


void TCPNetworkConnector::send(const SienaPlusMessage& msg) {
    int data_size = msg.ByteSize();
    int total_size = MSG_HEADER_SIZE + data_size;
    //FIXME: memory leak !
    unsigned char* arr_buf = new unsigned char[total_size];
    arr_buf[0] = BUFF_SEPERATOR;
    *((int*)(arr_buf + BUFF_SEPERATOR_LEN_BYTE)) = data_size;
    msg.SerializeWithCachedSizesToArray(arr_buf + MSG_HEADER_SIZE);
    send(arr_buf, total_size);
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
	return connect(tokens[1], port);
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
	log_debug("\nTCPConnector is connected.");
	start_read();
    return true;
    //start_sync_read();
}

void TCPNetworkConnector::disconnect() {
    boost::system::error_code ignored_ec;
    socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_->close();
}

bool TCPNetworkConnector::is_connected() {
	return socket_->is_open();
}

} /* namespace sienaplus */
