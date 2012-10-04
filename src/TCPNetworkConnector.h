/*
 * TCPNetworkConnector.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef TCPNetworkConnector_H_
#define TCPNetworkConnector_H_

#include "NetworkConnector.h"
#include <memory.h>

namespace sienaplus {

class TCPNetworkConnector: public sienaplus::NetworkConnector {
public:
	TCPNetworkConnector(boost::asio::io_service&, const std::function<void(NetworkConnector*, const char*, int)>&);
	TCPNetworkConnector(shared_ptr<boost::asio::ip::tcp::socket>&, const std::function<void(NetworkConnector*, const char*, int)>&);
	virtual ~TCPNetworkConnector();
	void async_connect(const string&, int);
	void async_connect(const string&);
	bool connect(const string&, int); //sync
	bool connect(const string&); // sync
	void disconnect();
	// data, length
	void send(const char*, size_t);
	void send(const string&);
	bool is_connected();
private:
	void connect_handler(const boost::system::error_code& error);
	void read_handler(const boost::system::error_code& ec, std::size_t bytes_num);
	void start_read();
    void write_handler(const boost::system::error_code& error, std::size_t bytes_transferred);
	//
	shared_ptr<boost::asio::ip::tcp::socket> socket_;
	// this flag specifies the behavior in case of connection termination, whether
	// a re-connection should be tried or not
	bool flag_try_reconnect;
};

} /* namespace sienaplus */
#endif /* TCPNetworkConnector_H_ */
