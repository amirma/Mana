/*
 * UDPMessageSender.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef UDPMessageSender_H_
#define UDPMessageSender_H_

#include <memory.h>
#include <thread>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include "MessageSender.h"
#include "exception"
#include "ManaException.h"
#include "Log.h"

namespace mana {

template <class T>
class UDPMessageSender: public MessageSender<T> {

public:
UDPMessageSender(const UDPMessageSender& other) = delete; // delete copy ctor
UDPMessageSender& operator=(const UDPMessageSender&) = delete; // delete assignment operator

/**
 * Construct an instance of this class with the given io_service.
 * This constructor is used when we need to manually create a connection.
 */
UDPMessageSender(boost::asio::io_service& srv, T& c, const URL& url) :
		MessageSender<T>(srv, c, url),
		remote_endpoint_(boost::asio::ip::address::from_string(url.address()), url.port()) {
			socket_ = make_shared<boost::asio::ip::udp::socket>(this->io_service_,
			boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
}

/**
 * Construct an instance of this class with the given socket.
 * This constructor is usually used when we have obtained a socket from an
 * acceptor that accepts a net connection. Note that the passed socket (skt)
 * has to outlive the instance of this class.
 */
UDPMessageSender(shared_ptr<boost::asio::ip::udp::socket>&& skt,
	T& c, const URL& url) : MessageSender<T>(skt->get_io_service(), c, url), socket_(skt) {
		//start_read();
	}

virtual ~UDPMessageSender() {}

/**
 * @brief Wait for the ongoing transmissions to finish and then
 * terminate the connection
 */
virtual void disconnect() {
    // TODO: check if there's an ongoing transmission...
    boost::system::error_code ignored_ec;
    socket_->shutdown(boost::asio::ip::udp::socket::shutdown_both, ignored_ec);
    socket_->close();
}

protected:

virtual void start_read() {{}

}

// private methods
private:

/**
 * This method has only internal purposes and is used to send a buffer of
 * a given size using the socket.
*/
virtual void send_buffer(const unsigned char* data, size_t length) {
	assert(this->write_buff_item_qu_.try_lock() == false);
    assert(data[0] == BUFF_SEPERATOR);
    assert(*((int*)(data + BUFF_SEPERATOR_LEN_BYTE)) + MSG_HEADER_SIZE ==  static_cast<long>(length));
    socket_->async_send_to(boost::asio::buffer(data, length), remote_endpoint_,
        this->write_hndlr_strand_.wrap(boost::bind(&UDPMessageSender<T>::write_handler,
        this, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)));
    FILE_LOG(logDEBUG3)  << "UDPMessageSender::send_buffer(): sent " << length << " bytes.";
}

// properties
shared_ptr<boost::asio::ip::udp::socket> socket_;
boost::asio::ip::udp::endpoint remote_endpoint_;
};

} /* namespace mana */
#endif /* UDPMessageSender_H_ */
