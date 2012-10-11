/*
 * NetworkConnector.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef NETWORKCONNECTOR_H_
#define NETWORKCONNECTOR_H_

#include <boost/asio.hpp>
#include "common.h"
#include "SienaPlusMessage.pb.h"
#include "MessageStream.h"
#include <functional>
#include <queue>


using namespace std;

namespace sienaplus {

struct WriteBufferItem {
    const unsigned char* data_;
    int size_;
};

// we put a shared data with its associated 
// mutex in one place, for easy management and less bugs.
struct WriteBufferItemQueueWrapper {
    PROTECTED_WITH(std::mutex);
    PROTECTED_MEMBER(queue<WriteBufferItem>, qu);
};

class NetworkConnector {
public:
	NetworkConnector(boost::asio::io_service&, const std::function<void(NetworkConnector*, SienaPlusMessage&)>&);
    NetworkConnector(const NetworkConnector&) = delete; // delete copy constructor
	virtual ~NetworkConnector();
    virtual void send(const SienaPlusMessage&) = 0;
	virtual void async_connect(const string&, int) = 0;
	virtual void async_connect(const string&) = 0;
	virtual bool connect(const string&, int) = 0; //sync
	virtual bool connect(const string&) = 0; // sync
    virtual void disconnect() = 0;
	virtual bool is_connected() = 0;
	int port() const;
	string& address() const;

protected:
	virtual void start_read() = 0;
	boost::asio::io_service& io_service_;
	std::function<void(NetworkConnector*, SienaPlusMessage&)> receive_handler;
	boost::asio::strand read_hndlr_strand_;
	boost::asio::strand write_hndlr_strand_;
    MessageStream message_stream_;
    array<unsigned char, MAX_MSG_SIZE> read_buffer_;
    WriteBufferItemQueueWrapper write_buff_item_qu_;
	int port_;
	string address_;
	bool flag_is_connected;
    bool flag_write_op_in_prog_;
};

} /* namespace sienaplus */
#endif /* NETWORKCONNECTOR_H_ */
