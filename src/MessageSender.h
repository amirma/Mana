/**
* @file MessageSender.h
* Interface for MessageSender
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
#include "MessageStream.h"
#include "URL.h"
#include "Log.h"

using namespace std;

namespace mana {

class ManaMessageProtobuf;

struct WriteBufferItem {
        const byte* data_;
        size_t offset_;
        size_t size_;
        bool flag_delete_; // if the flag is
        // set after sending this item, the buffer has to be
        // released.
    };

// we put a shared data with its associated
// mutex in one place, for easy management.
struct WriteBufferItemQueueWrapper {
	PROTECTED_WITH(std::mutex);
	PROTECTED_MEMBER(queue<WriteBufferItem>, qu);
};

/**
 * Type T is the user (owner) of an instance of the Session
 * class. This user is usually an end-host like a server (broker) or
 * a client.
 * */
template <class T>
class MessageSender {
public:
MessageSender(boost::asio::io_service& srv, T& c, const URL& url) :
		io_service_(srv), client_(c), url_(url), read_hndlr_strand_(srv), write_hndlr_strand_(srv),
		flag_is_connected(false), flag_write_op_in_prog_(false) {}

virtual ~MessageSender() {}

MessageSender(const MessageSender&) = delete; // delete copy ctor
MessageSender& operator=(const MessageSender&) = delete; // delete assig. operator

virtual void send_buffer(const byte* data, size_t length) = 0;

virtual void async_connect(const string&, int) {}
virtual void async_connect(const string&) {}
virtual bool connect(const URL&) {return true;}
virtual void disconnect() {}
virtual bool is_connected() {return true;}

/**
 * @brief Send a ManaMessageProtobuf out to the network.
 *
 * This is the only public interface for message transmission.
 * This method is thread-safe i.e., multiple threads can call
 * this method on the same object.
 *
 * Send is asynchronous and the message is converted to a buffer and the memory
 * is taken care of by the network connector. This means that after this method
 * returns the caller can safely reuse msg.
 */
void send(const ManaMessageProtobuf& msg) {
    // add buffer separators and header and then serialize
    // the message into a protobuf
    int data_size = msg.ByteSize();
    int total_size = MSG_HEADER_SIZE + data_size;
    if(total_size > MAX_MSG_SIZE) {
    	FILE_LOG(logWARNING) << "MessageSender::Send(): Message size is more than the allowed limit (" << MAX_MSG_SIZE << " Bytes). Message was discarded.";
        return;
    }
    byte* arr_buf = new byte[total_size];
    arr_buf[0] = BUFF_SEPERATOR;
    *((int*)(arr_buf + BUFF_SEPERATOR_LEN_BYTE)) = data_size;
    assert(*((int*)(arr_buf + BUFF_SEPERATOR_LEN_BYTE)) == data_size);
    assert(msg.SerializeWithCachedSizesToArray(arr_buf + MSG_HEADER_SIZE));
    if(!msg.SerializeWithCachedSizesToArray(arr_buf + MSG_HEADER_SIZE)) {
    	FILE_LOG(logERROR) << "MessageSender::Send(): Could not serialize message to buffer.";
        return;
    }
    prepare_buffer(arr_buf, total_size);
}

const URL& url() const {
  return this->url_;
}

protected:

/*  this method mutates 'write_buff_item_qu_.qu()' and so must be run by
 *  one thread at a time. This is guaranteed by using 'write_strand_'. Though
 *  this is not enough to serialize access to write_buff_item_qu_ because
 *  the 'send' method also mutates this object. Hence we need lock the
 *  object.
 */
void write_handler(const boost::system::error_code& error, std::size_t bytes_transferred) {
    lock_guard<WriteBufferItemQueueWrapper> lock(this->write_buff_item_qu_);
    if(error) {
    	FILE_LOG(logERROR) << "MessageSender::write_handler(): Error sending data: " << error.message();
    } else {
    	FILE_LOG(logDEBUG3) << "MessageSender::write_handler(): wrote " << bytes_transferred << " bytes.";
    	FILE_LOG(logDEBUG4) << "MessageSender::write_handler(): wrote " << this->write_buff_item_qu_.qu().front().data_;
    }
    assert(this->write_buff_item_qu_.qu().empty() == false); // at least the last
    // buffer that was written must be in the queue
    assert(this->write_buff_item_qu_.qu().front().size_ != 0);
    // delete the memory and remove the item from the queue, if this was
    // the last item in a trail of buffers.
    if(this->write_buff_item_qu_.qu().front().flag_delete_)
    	delete[] this->write_buff_item_qu_.qu().front().data_;
    this->write_buff_item_qu_.qu().pop();
    // if there's more items in the queue waiting to be written
    // to the socket continue sending ...
    if(this->write_buff_item_qu_.qu().empty() == false) {
    	const auto& tmp = this->write_buff_item_qu_.qu().front();
        send_buffer(tmp.data_ + tmp.offset_, tmp.size_);
    }
}

// private methods
// class properties
boost::asio::io_service& io_service_;
T& client_;
const URL url_;
boost::asio::strand read_hndlr_strand_;
boost::asio::strand write_hndlr_strand_;
bool flag_is_connected;
bool flag_write_op_in_prog_;
MessageStream message_stream_;
array<byte, MAX_MSG_SIZE> read_buffer_;
WriteBufferItemQueueWrapper write_buff_item_qu_;
mutex read_buff_mutex_;

private:
/*WriteBufferItem item;
 * Prepares a buffer for transmission over the network.
 * If there is a transmission going on already, the buffer will be queued for
 * later transmission. Otherwise the buffer is sent out without being queued.
 *
 * Note that this method mutates the 'write_buff_item_qu_.qu()' and so must be
 * called by one thread only. This is guaranteed by using strand
 * @param data data A pointer to the buffer
 * @param length length of the buffer
 */
void prepare_buffer(const byte* data, size_t length) {
	FILE_LOG(logDEBUG3) << "MessageSender::prepare_buffer(): preparing " << length << " bytes.";
    lock_guard<WriteBufferItemQueueWrapper> lock(this->write_buff_item_qu_);
    assert(length > 0);
    bool flg_send_not_in_progress = this->write_buff_item_qu_.qu().empty();
    // if the message size is more that the limit we have to break it into
    // multiple sends. Hence the loop.
    size_t offset = 0;
    do {
    	WriteBufferItem item;
    	if(length > MAX_PCKT_SIZE)
    		item.size_ = MAX_PCKT_SIZE;
    	else
    		item.size_ = length;
    	item.data_ = data;
    	item.offset_ = offset;
    	item.flag_delete_ = false;
    	length -= item.size_;
    	offset += item.size_;
    	this->write_buff_item_qu_.qu().push(item);
    } while(length > 0);
    // the last item in this series of items must have its delete flag set to one
    // so after sending it, we know we must freed the buffer.
    this->write_buff_item_qu_.qu().back().flag_delete_ = true;
    if(flg_send_not_in_progress) {
    	const auto& tmp = this->write_buff_item_qu_.qu().front();
    	send_buffer(tmp.data_ + tmp.offset_, tmp.size_);
    }
    assert(!this->write_buff_item_qu_.qu().empty());
    assert(this->write_buff_item_qu_.qu().front().size_ != 0);
}

};

} /* namespace mana */
#endif /* NETWORKCONNECTOR_H_ */
