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
virtual void send_buffer(const unsigned char* data, size_t length) = 0;
virtual void async_connect(const string&, int) = 0;
virtual void async_connect(const string&) = 0;
virtual bool connect(const URL&) = 0; //sync
virtual void disconnect() = 0;
virtual bool is_connected() = 0;
//virtual void start() {}

/**
 * @brief Send a SienPlusMessage out to the network.
 *
 * This is the only public interface for message transmission.
 * This method is thread-safe i.e., multiple threads can call
 * this method on the same object.
 *
 * Send is asynchronous and the message is converted to a buffer and the memory
 * is taken care of by the network connector. This means that after this method
 * returns the caller can safely reuse msg.
 */
void send(const ManaMessage& msg) {
    // add buffer separators and header and then serialize
    // the message into a protobuf
    int data_size = msg.ByteSize();
    int total_size = MSG_HEADER_SIZE + data_size;
    unsigned char* arr_buf = new unsigned char[total_size];
    arr_buf[0] = BUFF_SEPERATOR;
    *((int*)(arr_buf + BUFF_SEPERATOR_LEN_BYTE)) = data_size;
    assert(*((int*)(arr_buf + BUFF_SEPERATOR_LEN_BYTE)) == data_size);
    assert(msg.SerializeWithCachedSizesToArray(arr_buf + MSG_HEADER_SIZE));
    if(!msg.SerializeWithCachedSizesToArray(arr_buf + MSG_HEADER_SIZE)) {
    	log_err("NetworkConnector::Send(): Could not serialize message to buffer.");
        return;
    }
    prepare_buffer(arr_buf, total_size);
}

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
    /*
    struct bla {
    	private:
    		std::mutex mut_;
    		array<unsigned char, MAX_MSG_SIZE> buffer_;
		public:
    		void    lock()         {  mut_.lock();  }
    		bool    try_lock()    {  return mut_.try_lock();  }
			void    unlock()      {  mut_.unlock();  }
			array<unsigned char, MAX_MSG_SIZE>&   buffer()     { assert(!mut_.try_lock()); return buffer_; }
			const 	array<unsigned char, MAX_MSG_SIZE>&    const_buffer()     { return buffer_; }
    } read_buffer_;*/

    WriteBufferItemQueueWrapper write_buff_item_qu_;
    mutex read_buff_mutex_;

/*  this method mutates 'write_buff_item_qu_.qu()' and so must be run by
 *  one thread at a time. This is guaranteed by using 'write_strand_'. Though
 *  this is not enough to serialize access to write_buff_item_qu_ because
 *  the 'send' method also mutates this object. Hence we need lock the
 *  object.
 */
void write_handler(const boost::system::error_code& error, std::size_t bytes_transferred) {
    lock_guard<WriteBufferItemQueueWrapper> lock(this->write_buff_item_qu_);
    log_debug("NetworkConnector::write_handler(): wrote " << bytes_transferred << " bytes: " << this->write_buff_item_qu_.qu().front().data_);
    assert(this->write_buff_item_qu_.qu().empty() == false); // at least the last
    // buffer that was written must be in the queue
    assert(this->write_buff_item_qu_.qu().front().size_ != 0);
    // delete the memory and remove the item from the queue
    delete[] this->write_buff_item_qu_.qu().front().data_;
    this->write_buff_item_qu_.qu().pop();
    // if there's more items in the queue waiting to be written
    // to the socket continue sending ...
    if(this->write_buff_item_qu_.qu().empty() == false)
        send_buffer(this->write_buff_item_qu_.qu().front().data_, this->write_buff_item_qu_.qu().front().size_);
}

void read_handler(const boost::system::error_code& ec, std::size_t bytes_num) {
	if(!ec && ec.value() != 0) {
		log_err("TCPNetworkConnector::read_handler(): error reading:" << ec.message());
		return;
	}
	// TODO: i'm using the number of bytes as a hint that the connection
    // terminated. I'm not sure this is a good way though. For some
    // reason socket.is_open() does not do it's job...
	if(bytes_num == 0) {
        this->flag_is_connected = false;
        log_debug("TCPNetworkConnector::read_handler(): connection seems to be closed.");
        return;
	}
    log_debug("TCPNetworkConnector::read_handler(): read " << bytes_num << " bytes.");

    ManaMessage msg;
    // Note: message_stream MUST be accessed by only one thread at a time - it's
    // not thread safe. Here the assumption is that read_handler is run only
    // by one thread at a time. This is guaranteed by using strand_ for async
    // read.
    this->message_stream_.consume(this->read_buffer_.data(), bytes_num);
    while(this->message_stream_.produce(msg))
    	this->client_.handle_message(*this, msg);

    if(is_connected())
        start_read();
}

private:
/*
 * Prepares a buffer for transmission over the network.
 * If there is a transmission going on already, the buffer will be queued for
 * later transmission. Otherwise the buffer is sent out without being queued.
 *
 * Note that this method mutates the 'write_buff_item_qu_.qu()' and so must be
 * called by one thread only. This is guaranteed by using strand
 * @param data data A pointer to the buffer
 * @param length length of the buffer
 */
void prepare_buffer(const unsigned char* data, size_t length) {
    lock_guard<WriteBufferItemQueueWrapper> lock(this->write_buff_item_qu_);
    assert(length != 0);
    WriteBufferItem item;
    item.data_ = data;
    item.size_ = length;
    bool flg_send_not_in_progress = this->write_buff_item_qu_.qu().empty();
    this->write_buff_item_qu_.qu().push(item);
    if(flg_send_not_in_progress)
        send_buffer(item.data_, item.size_);
    assert(!this->write_buff_item_qu_.qu().empty());
    assert(this->write_buff_item_qu_.qu().front().size_ != 0);
}

};

} /* namespace mana */
#endif /* NETWORKCONNECTOR_H_ */
