/*
 * TCPNetworkConnector.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include <thread>
#include <boost/algorithm/string.hpp>
#include "boost/bind.hpp"
#include "exception"
#include "ManaException.h"
#include "TCPNetworkConnector.h"


namespace mana {

/*
 * Construct an instance of this class with the given io_service.
 * This constructor is used when we need to manually create a connection.
 */
template <class T>
TCPNetworkConnector<T>::TCPNetworkConnector(boost::asio::io_service& srv, T& c) :
		NetworkConnector<T>(srv, c) {
	socket_ = make_shared<boost::asio::ip::tcp::socket>(this->io_service_);
}

/*
 * Construct an instance of this class with the given socket.
 * This constructor is usually used when we have obtained a socket from an
 * acceptor that accepts a net connection. Note that the passed socket (skt)
 * has to outlive the instance of this class.
 */
template <class T>
TCPNetworkConnector<T>::TCPNetworkConnector(shared_ptr<boost::asio::ip::tcp::socket>&& skt,
	T& c) : NetworkConnector<T>(skt->get_io_service(), c), socket_(skt) {
	if(socket_->is_open()) {
		this->flag_is_connected = true;
        set_socket_options();
		start_read();
	}
}

template <class T>
TCPNetworkConnector<T>::~TCPNetworkConnector() {
}

template <class T>
void TCPNetworkConnector<T>::set_socket_options() {
#ifdef DISABLE_NAGLE_ALG
    //disable the Nagle's algorithm so that small-sized messages
    //will be pushed as soon as send is called
    boost::asio::ip::tcp::no_delay option_op(true);
    socket_->set_option(option_op);
#endif
}

template <class T>
void TCPNetworkConnector<T>::async_connect(const string& url) {
	if(this->flag_is_connected)
		return;
	vector<string> tokens;
	boost::split(tokens, url, boost::is_any_of(":"));
	int port = 0;
	try {
		port = stoi(tokens[2]);
	} catch(exception& e) {
		throw ManaException("Could not connect: invalid port number: " + tokens[2]);
	}
	async_connect(tokens[1], port);
}

template <class T>
void TCPNetworkConnector<T>::async_connect(const string& addr, int prt) {
	if(this->flag_is_connected)
		return;
	try {
		boost::asio::ip::tcp::endpoint endpnt(boost::asio::ip::address::from_string(addr), prt);
		socket_->async_connect(endpnt, boost::bind(&TCPNetworkConnector<T>::connect_handler, this, boost::asio::placeholders::error));
	} catch(exception& e) {
		this->flag_is_connected = false;
		log_info("count not connect");
	}
}

/*  this method mutates 'write_buff_item_qu_.qu()' and so must be run by
 *  one thread at a time. This is guaranteed by using 'write_strand_'
 *  */
template <class T>
void TCPNetworkConnector<T>::write_handler(const boost::system::error_code& error, std::size_t bytes_transferred) {
    lock_guard<WriteBufferItemQueueWrapper> lock(this->write_buff_item_qu_);
    if(this->write_buff_item_qu_.qu().empty())
        return;
    log_debug("\nTCPNetworkConnector<T>::write_handler(): wrote " << bytes_transferred << " bytes.");
    assert(!this->write_buff_item_qu_.qu().empty()); // at least the last
    // buffer that was written must be in the queue
    assert(this->write_buff_item_qu_.qu().front().size_ != 0);
    // delete the memory and remove the item from the queue
    delete[] this->write_buff_item_qu_.qu().front().data_;
    this->write_buff_item_qu_.qu().pop();
    // if there's more items in the queue waiting to be written
    // to the socket continute sending ...
    if(!this->write_buff_item_qu_.qu().empty())
        send_buffer(this->write_buff_item_qu_.qu().front().data_, this->write_buff_item_qu_.qu().front().size_);
}

template <class T>
void TCPNetworkConnector<T>::connect_handler(const boost::system::error_code& ec) {
	if(!ec && ec.value() != 0) {
	    log_info("\ncTCPNetworkConnector<T>::connect_handler: could not connect.");
        return;
	}
    set_socket_options();
	this->flag_is_connected = true;
	start_read();
}

template <class T>
void TCPNetworkConnector<T>::start_read() {
    socket_->async_read_some(boost::asio::buffer(this->read_buffer_, MAX_MSG_SIZE),
            this->read_hndlr_strand_.wrap(boost::bind(&TCPNetworkConnector<T>::read_handler, this,
    	boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

template <class T>
void TCPNetworkConnector<T>::read_handler(const boost::system::error_code& ec, std::size_t bytes_num) {
	if(!ec && ec.value() != 0) {
		log_err("error reading:" << ec.message());
		return;
	}
	// TODO: i'm using the number of bytes as a hint that the connection
    // terminated. I'm not sure this is a good way though. For some
    // reason socket.is_open() does not do it's job...
	if(bytes_num == 0) {
        this->flag_is_connected = false;
        log_debug("\nTCPNetworkConnector<T>::read_handler(): connection seems to be closed.");
        return;
	}
    log_debug("\nTCPNetworkConnector<T>::read_handler(): read " << bytes_num << " bytes.");

    ManaMessage msg;
    // Note: message_stream MUST be acessed by only one thread at a time - it's
    // not thread safe. Here the assumption is that read_handler is run only
    // by one thread at a time. This is guaranteed by using strand_ for async
    // read.
    this->message_stream_.consume(this->read_buffer_.data(), bytes_num);
    while(this->message_stream_.produce(msg))
    this->client_.handle_message(*this, msg);

    if(is_connected())
        start_read();
}

template <class T>
void TCPNetworkConnector<T>::start_sync_read() {
    std::thread([&](){
        for(;;) {
            try {
            size_t bytes_num = socket_->read_some(boost::asio::buffer(this->read_buffer_, MAX_MSG_SIZE));
            ManaMessage msg;
            this->message_stream_.consume(this->read_buffer_.data(), bytes_num);
            while(this->message_stream_.produce(msg))
        	    receive_handler(this, msg);
            log_debug("\nTCPNetworkConnector<T>::start_sync_read: read " << bytes_num << " bytes.");
            } catch(exception& e) {
                disconnect();
            }
        }
    }).detach();
}

/**
 * @brief this method had only internal purposes and is used to send a buffer of given
*  size to the socket
*/
template <class T>
void TCPNetworkConnector<T>::send_buffer(const unsigned char* data, size_t length) {
	if(!is_connected()) {
		log_err("TCPNetworkConnector<T>::send_buffer(): socket is not connected. not sending.");
		return;
	}
    assert(data[0] == BUFF_SEPERATOR);
    assert(*((int*)(data + BUFF_SEPERATOR_LEN_BYTE)) + MSG_HEADER_SIZE ==  static_cast<long>(length));
    boost::asio::async_write(*socket_, boost::asio::buffer(data, length),
        this->write_hndlr_strand_.wrap(boost::bind(&TCPNetworkConnector<T>::write_handler,
        this, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)));
    log_debug("\nTCPNetworkConnector<T>::send_buffer(): sent " << length << " bytes.");
}

/**
 * @brief For internal purposes only. Prepares a buffer for transmission over
 * the network.
 *
 * If there is a transmission going on already, the buffer will be queued for
 * later transmission. Otherwise the buffer is sent out without being queued.
 *
 * Note that this method mutates the 'write_buff_item_qu_.qu()' and so must be
 * called by one thread only. This is guaranteed by using strand
 * @param data data A pointer to the buffer
 * @param length length of the buffer
 */
template <class T>
void TCPNetworkConnector<T>::prepare_buffer(const unsigned char* data, size_t length) {
    lock_guard<WriteBufferItemQueueWrapper> lock(this->write_buff_item_qu_);
    assert(length != 0);
    WriteBufferItem item;
    item.data_ = data;
    item.size_ = length;
    bool flg_send_not_in_progress = this->write_buff_item_qu_.qu().empty();
    this->write_buff_item_qu_.qu().push(item);
    if(flg_send_not_in_progress)
        send_buffer(item.data_, item.size_);
    assert(!this->write_buff_item_qu_.qu().empty());
    assert(this->write_buff_item_qu_.qu().front().size_ != 0);
}

/**
 * @brief Send a SienPlusMessage out to the network. This is the only public
 * interface for message transmission.
 *
 * Send is asynchronous and the message is converted to a buffer and the memory
 * is taken care of by the network connector. This means that after this method
 * returns the caller can safely reuse msg.
 */
template <class T>
void TCPNetworkConnector<T>::send(const ManaMessage& msg) {
    // add buffer separators and header and then serialize
    // the message into a protobuf
    int data_size = msg.ByteSize();
    int total_size = MSG_HEADER_SIZE + data_size;
    unsigned char* arr_buf = new unsigned char[total_size];
    arr_buf[0] = BUFF_SEPERATOR;
    *((int*)(arr_buf + BUFF_SEPERATOR_LEN_BYTE)) = data_size;
    assert(*((int*)(arr_buf + BUFF_SEPERATOR_LEN_BYTE)) == data_size);
    if(!msg.SerializeWithCachedSizesToArray(arr_buf + MSG_HEADER_SIZE)) {
        log_err("\nTCPNetworkConnector<T>::send(): Could not serialize message to buffer.");
        return;
    }
    prepare_buffer(arr_buf, total_size);
}

template <class T>
bool TCPNetworkConnector<T>::connect(const string& url) {
	if(is_connected()) {
		return true;
	}
	vector<string> tokens;
	boost::split(tokens, url, boost::is_any_of(":"));
	int port = 0;
	try {
		port = stoi(tokens[2]);
	} catch(exception& e) {
		throw ManaException("Could not connect: invalid port number: " + tokens[2]);
	}
	return connect(tokens[1], port);
}

template <class T>
bool TCPNetworkConnector<T>::connect(const string& addr, int prt) {
	if(is_connected()) {
		return true;
	}
	try {
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(addr), prt);
		socket_->connect(endpoint);
	} catch(boost::system::system_error& e) {
		throw ManaException("Error connecting to endpoint.");
	} catch(exception& e) {
		throw ManaException("Error connecting to endpoint.");
	}
	//
	this->flag_is_connected = true;
	log_debug("\nTCPConnector is connected.");
	start_read();
    return true;
}

/**
 * @brief Wait for the ongoing transmissions to finish and then
 * terminate the connection
 */
template <class T>
void TCPNetworkConnector<T>::disconnect() {
    // TODO: check if there's an ongoing transmission...
    boost::system::error_code ignored_ec;
    socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_->close();
}

template <class T>
bool TCPNetworkConnector<T>::is_connected() {
	return socket_->is_open();
}

template <class T>
void *  TCPNetworkConnector<T>::asio_handler_allocate(std::size_t size) {
    return new unsigned char[size];
}

template <class T>
void TCPNetworkConnector<T>::asio_handler_deallocate(void * pointer, std::size_t size) {

}

} /* namespace mana */
