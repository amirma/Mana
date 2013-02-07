/*
 * UDPMessageReceiver.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef UDPMESSAGERECEIVER_H_
#define UDPMESSAGERECEIVER_H_

#include "MessageReceiver.h"
#include <functional>
#include <string>

using namespace std;

namespace mana {

template <class T>
class UDPMessageReceiver: public MessageReceiver<T> {
public:

	UDPMessageReceiver(boost::asio::io_service& srv, T& client, const URL& url) :
		MessageReceiver<T>(srv, client, url) {
		this->connection_type_ = mana::udp;
	}

virtual ~UDPMessageReceiver() {}

virtual void start() {
	/*
	acceptor_ptr_ = make_shared<boost::asio::ip::tcp::acceptor>(this->io_service_);
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), this->url_.port());
	acceptor_ptr_->open(endpoint.protocol());
	acceptor_ptr_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_ptr_->bind(endpoint);
	acceptor_ptr_->listen();
	this->flag_runing_ = true;
	begin_accept();
	*/
}

virtual void stop() {}

private:

};

} /* namespace mana */
#endif /* UDPMESSAGERECEIVER_H_ */
