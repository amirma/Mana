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

namespace mana {

class TCPNetworkConnector: public mana::NetworkConnector {
public:
	TCPNetworkConnector(boost::asio::io_service&, const std::function<void(NetworkConnector*, ManaMessage&)>&);
	TCPNetworkConnector(shared_ptr<boost::asio::ip::tcp::socket>&, const std::function<void(NetworkConnector*, ManaMessage&)>&);
    TCPNetworkConnector(const TCPNetworkConnector& other) = delete; // delete copy constructor
    TCPNetworkConnector& operator=(const TCPNetworkConnector&) = delete; // delete assignment operator
    virtual ~TCPNetworkConnector();
	void async_connect(const string&, int);
	void async_connect(const string&);
	bool connect(const string&, int); //sync
	bool connect(const string&); // sync
	void disconnect();
	// data, length
    void send(const ManaMessage&);
	bool is_connected();
    void * asio_handler_allocate(std::size_t size);
    void asio_handler_deallocate(void * pointer, std::size_t size);

private:
	void connect_handler(const boost::system::error_code& error);
	void read_handler(const boost::system::error_code& ec, std::size_t bytes_num);
	void start_read();
    void start_sync_read();
    void write_handler(const boost::system::error_code& error, std::size_t bytes_transferred);
    void set_socket_options();
	void prepare_buffer(const unsigned char*, size_t);
	void send_buffer(const unsigned char*, size_t);
	//
	shared_ptr<boost::asio::ip::tcp::socket> socket_;
	// this flag specifies the behavior in case of connection termination, whether
	// a re-connection should be tried or not
	bool flag_try_reconnect;
};

} /* namespace mana */
#endif /* TCPNetworkConnector_H_ */
