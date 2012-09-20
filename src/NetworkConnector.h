/*
 * NetworkConnector.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef NETWORKCONNECTOR_H_
#define NETWORKCONNECTOR_H_

#include<boost/asio.hpp>
#include "SienaPlusMessage.pb.h"
#include <functional>

using namespace std;

namespace sienaplus {

class NetworkConnector {
public:
	NetworkConnector(boost::asio::io_service&, std::function<void(const char*, int size)>);
	virtual ~NetworkConnector();
	virtual void send(const char*, int) = 0;
	virtual void send(const string&) = 0;
	virtual void async_connect(const string&, int) = 0;
	virtual void async_connect(const string&) = 0;
	virtual bool connect(const string&, int) = 0; //sync
	virtual bool connect(const string&) = 0; // sync
	virtual bool is_connected() = 0;
	int port() const;
	string& address() const;

protected:
	virtual void start_read() = 0;
	boost::asio::io_service& io_service_;
	int port_;
	string address_;
	bool flag_is_connected;
	array<char, 8192> read_buffer_;
	std::function<void(const char*, int size)> receive_handler;
	boost::asio::strand strand_;
};

} /* namespace sienaplus */
#endif /* NETWORKCONNECTOR_H_ */
