/*
 * UDPMessageReceiver.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef UDPMESSAGERECEIVER_H_
#define UDPMESSAGERECEIVER_H_

#include <functional>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "MessageReceiver.h"

using namespace std;

namespace mana {

template <class T>
class UDPMessageReceiver: public MessageReceiver<T> {
public:

	UDPMessageReceiver(boost::asio::io_service& srv, T& client, const URL& url):
		MessageReceiver<T>(srv, client, url) {
		this->connection_type_ = mana::udp;
	}

virtual ~UDPMessageReceiver() {}

virtual void start() override {
	boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), this->url_.port());
	socket_ = make_shared<boost::asio::ip::udp::socket>(this->io_service_, endpoint);
	start_read();
	this->flag_runing_ = true;
}

virtual void stop() override {}

virtual connection_type transport_type() const {
	return connection_type::udp;
}

private:

void start_read() {
    socket_->async_receive_from(boost::asio::buffer(this->read_buffer_, MAX_MSG_SIZE),
    		all_endpoints_,
    		this->read_hndlr_strand_.wrap(boost::bind(&UDPMessageReceiver<T>::read_handler, this,
    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void read_handler(const boost::system::error_code& ec, std::size_t bytes_num) {
	if(!ec && ec.value() != 0) {
		log_err("UDPMessageSender::read_handler(): error reading:" << ec.message());
		return;
	}
	// FIXME: i'm using the number of bytes as a hint that the connection
    // terminated. I'm not sure this is a good way though. For some
    // reason socket.is_open() does not do it's job...
	// FIXME: this check is just blindly copies and pasted here from TCPMessageSender.
	// I guess it wont even have a meaning here .... gotta check this.
	if(bytes_num == 0) {
        log_debug("UDPMessageSender::read_handler(): connection seems to be closed.");
        return;
	}
    log_debug("UDPMessageSender::read_handler(): read " << bytes_num << " bytes.");

    ManaMessage msg;
    // Note: message_stream MUST be accessed by only one thread at a time - it's
    // not thread safe. Here the assumption is that read_handler is run only
    // by one thread at a time. This is guaranteed by using strand_ for async
    // read.
    this->message_stream_.consume(this->read_buffer_.data(), bytes_num);
    while(this->message_stream_.produce(msg))
    	this->client_.handle_message(msg, this);

    start_read();
}

	// properties
    shared_ptr<boost::asio::ip::udp::socket> socket_;
	boost::asio::ip::udp::endpoint all_endpoints_;
};

} /* namespace mana */
#endif /* UDPMESSAGERECEIVER_H_ */
