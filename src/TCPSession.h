/**
 * TCPSession.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef TCPSession_H_
#define TCPSession_H_

#include <memory.h>
#include "boost/bind.hpp"
#include "ManaException.h"
#include "Log.h"

namespace mana {

//  forward declaration for a pointer we will have later
template <class T>
class TCPMessageReceiver;

template <class T>
class TCPSession : private MessageReceiver<T> {

public:
    TCPSession(const TCPSession& other) = delete; // delete copy ctor
    TCPSession& operator=(const TCPSession&) = delete; // delete assignment operator
/*
 * Construct an instance of this class with the given io_service.
 * This constructor is used when we need to manually create a connection.
 */
TCPSession(const shared_ptr<boost::asio::ip::tcp::socket>& s, T& c, TCPMessageReceiver<T>* mr) :
	MessageReceiver<T>(mr->io_service(), c, mr->url()),
	socket_(s), client_(c), message_receiver_(mr) {
	start_read();

}

virtual ~TCPSession() {
	disconnect();
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
 * @brief Wait for the ongoing transmissions to finish and then
 * terminate the connection
 */
void disconnect() {
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

virtual void start(){}
virtual void stop(){}
virtual connection_type transport_type() const {
	return connection_type::tcp;
}

void read_handler(const boost::system::error_code& ec, std::size_t bytes_num) {
	if(!ec && ec.value() != 0) {
		FILE_LOG(logDEBUG2) << "TCPSession::read_handler(): error reading:" << ec.message();
		return;
	}
	// TODO: i'm using the number of bytes as a hint that the connection
    // terminated. I'm not sure this is a good way though. For some
    // reason socket.is_open() does not do it's job...
	if(bytes_num == 0) {
        //this->flag_is_connected = false;
        FILE_LOG(logDEBUG2) << "TCPSession::read_handler(): connection seems to be closed.";
        return;
	}
	FILE_LOG(logDEBUG2) << "TCPSession::read_handler(): read " << bytes_num << " bytes.";
    ManaMessage msg;
    // Note: message_stream MUST be accessed by only one thread at a time - it's
    // not thread safe. Here the assumption is that read_handler is run only
    // by one thread at a time. This is guaranteed by using strand_ for async
    // read.
    this->message_stream_.consume(this->read_buffer_.data(), bytes_num);
    while(this->message_stream_.produce(msg)) {
    	this->client_.handle_message(msg, message_receiver_);
    	msg.Clear();
    }

    if(is_connected())
    	start_read();
}

void start_read() {
    socket_->async_read_some(boost::asio::buffer(this->read_buffer_, MAX_MSG_SIZE),
            this->read_hndlr_strand_.wrap(boost::bind(&TCPSession<T>::read_handler, this,
    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

const shared_ptr<boost::asio::ip::tcp::socket> socket_;
T& client_;
TCPMessageReceiver<T>* message_receiver_;
};

} /* namespace mana */
#endif /* TCPSession_H_ */
