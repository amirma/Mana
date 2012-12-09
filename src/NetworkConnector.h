/*
 * NetworkConnector.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef NETWORKCONNECTOR_H_
#define NETWORKCONNECTOR_H_

#include <functional>
#include <queue>
#include <array>
#include <boost/asio.hpp>
#include "common.h"
#include "ManaMessage.pb.h"
#include "MessageStream.h"

using namespace std;

namespace mana {

struct WriteBufferItem {
        const unsigned char* data_;
        int size_;
    };

// we put a shared data with its associated
// mutex in one place, for easy management.
struct WriteBufferItemQueueWrapper {
	PROTECTED_WITH(std::mutex);
	PROTECTED_MEMBER(queue<WriteBufferItem>, qu);
};

template <class T>
class NetworkConnector {
public:
NetworkConnector(boost::asio::io_service& srv, T& c) :
		io_service_(srv), client_(c), port_(0), address_(""), read_hndlr_strand_(srv), write_hndlr_strand_(srv),
                flag_is_connected(false), flag_write_op_in_prog_(false) {}

virtual ~NetworkConnector() {}

NetworkConnector(const NetworkConnector&) = delete; // delete copy ctor
NetworkConnector& operator=(const NetworkConnector&) = delete; // delete assig. operator
virtual void send(const ManaMessage&) = 0;
virtual void async_connect(const string&, int) = 0;
virtual void async_connect(const string&) = 0;
virtual bool connect(const string&, int) = 0; //sync
virtual bool connect(const string&) = 0; // sync
virtual void disconnect() = 0;
virtual bool is_connected() = 0;

int port() const {
    return port_;
}

string& address() const {
    return address_;
}

protected:
    // private methods
    virtual void start_read() = 0;
    // class properties
    boost::asio::io_service& io_service_;
    T& client_;
    int port_;
    string address_;
    boost::asio::strand read_hndlr_strand_;
    boost::asio::strand write_hndlr_strand_;
    bool flag_is_connected;
    bool flag_write_op_in_prog_;
    MessageStream message_stream_;
    array<unsigned char, MAX_MSG_SIZE> read_buffer_;
    WriteBufferItemQueueWrapper write_buff_item_qu_;
};

} /* namespace mana */
#endif /* NETWORKCONNECTOR_H_ */
