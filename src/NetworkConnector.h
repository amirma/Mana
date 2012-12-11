/**
* @file NetworkConnector.h
* Interface for NetworkConnector
*
* @author Amir Malekpour
* @version 0.1
*
* Copyright © 2012 Amir Malekpour
*
*  Mana is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  Mana is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*  FOR A PARTICULAR PURPOSE. For more details see the GNU General Public License
*  at <http: *www.gnu.org/licenses/>
**/

#ifndef NETWORKCONNECTOR_H_
#define NETWORKCONNECTOR_H_

#include <functional>
#include <queue>
#include <array>
#include <boost/asio.hpp>
#include "common.h"
#include "ManaMessage.pb.h"
#include "MessageStream.h"
#include "URL.h"

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
NetworkConnector(boost::asio::io_service& srv, T& c, const URL& url) :
		io_service_(srv), client_(c), url_(url), read_hndlr_strand_(srv), write_hndlr_strand_(srv),
                flag_is_connected(false), flag_write_op_in_prog_(false) {}

virtual ~NetworkConnector() {}

NetworkConnector(const NetworkConnector&) = delete; // delete copy ctor
NetworkConnector& operator=(const NetworkConnector&) = delete; // delete assig. operator
virtual void send(const ManaMessage&) = 0;
virtual void async_connect(const string&, int) = 0;
virtual void async_connect(const string&) = 0;
virtual bool connect(const URL&) = 0; //sync
virtual void disconnect() = 0;
virtual bool is_connected() = 0;


const URL& url() const {
    return this->url_;
}

protected:
    // private methods
    virtual void start_read() = 0;
    // class properties
    boost::asio::io_service& io_service_;
    T& client_;
    const URL url_;
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
