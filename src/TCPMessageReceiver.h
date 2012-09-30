/*
 * TCPMessageReceiver.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef TCPMESSAGERECEIVER_H_
#define TCPMESSAGERECEIVER_H_

#include "MessageReceiver.h"
#include "Broker.h"
#include <boost/asio.hpp>
#include <vector>
#include <functional>
#include "string"
#include "TCPNetworkConnector.h"


using namespace std;

namespace sienaplus {

class TCPMessageReceiver: public sienaplus::MessageReceiver {
public:
	// port_number, ip_address
	TCPMessageReceiver(boost::asio::io_service&, const int, const string&,
			const std::function<void(const char*, int)>&,
            const std::function<void(shared_ptr<NetworkConnector>)>&);
	virtual ~TCPMessageReceiver();
	void start();
	void stop();
private:
	void begin_accept();
	void accept_handler(const boost::system::error_code&);
	int port;
	shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_ptr_;
	vector<shared_ptr<boost::asio::ip::tcp::socket> >remote_endpoints;
	vector<shared_ptr<TCPNetworkConnector> > tcp_connections;
	shared_ptr<boost::asio::ip::tcp::socket> next_connection_socket_;
	int port_;
	string address_;
    std::function<void(shared_ptr<NetworkConnector>)> connect_handler_; //this
    // callback is provided by the owner of this acceptor. when a new
    // connwction is established this we invoke this callback.
};

} /* namespace sienaplus */
#endif /* TCPMESSAGERECEIVER_H_ */
