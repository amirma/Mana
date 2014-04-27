#include "MessageStream.h"
#include "Log.h"

namespace mana {

MessageStream::MessageStream() :
	unconsumed_data_size_(0), new_data_(nullptr),
    new_data_size_(0) {}

MessageStream::~MessageStream() {}

void MessageStream::consume(const byte* buff, int size) {
    assert(new_data_size_ == 0); // there must not be any
    // data remained from before. If this assertion fails,
    // either the MessageStream::consume() is not called enough
    // times to consume all the available data in the previous new_data_
    // buffer. This can happen, for instance, when two separate threads,
    // access the same instance of MessageStream.
    new_data_ = buff;
    new_data_size_ = size;
    FILE_LOG(logDEBUG2)  << "MessageStream:consume(): received new buffer. Buffer size: " << size;
}

/*
 * Take a buffer 'data' with a maximum size of 'size' and de-serialize into msg
 * and put the number of consumed bytes in 'consumed_size', return true on
 * success. If false is returned, we should always check (in the caller of this
 * function) the value of 'consumed_size'. This value being zero indicates that
 * the buffer was potentially incomplete (i.e, the end of a received buffer
 * from the socket), so nothing was consumed and data was left for later
 * consumption, when the rest of the buffer arrives from the socket. If the
 * value is non-zero it means there was an error in the buffer and some bytes
 * were skipped (consumed).
 */
bool MessageStream::do_produce(const byte* data, int size, ManaMessageProtobuf& msg, int& consumed_size) const {
    assert(size > 0);
    assert(data[0] == BUFF_SEPERATOR);// this should not fail unless
    // received data is corrupted somehow. In debug build we must fail
    // and try to debug. For fault tolerance purposes or when we are
    // using UDP and no checksumming is done we need to be prepared for
    // anything, hence the following check...
    if(data[0] != BUFF_SEPERATOR) {
        // seems like we got rubbish in the buffer, lets skip 
        // to either the end of the buffer of to the beginning of 
        // a new message.
        int i = 0;
        while(i < size && data[i] != BUFF_SEPERATOR)
            i++;
        consumed_size = i;
        FILE_LOG(logWARNING)  << "MessageStream::do_produce():" << __LINE__ <<  " : corrupted data was received. Discarded bytes: " << i	;
        return false;
    }
    //if 'size' is less than the length of a header we return
    if(size < MSG_HEADER_SIZE) {
        consumed_size = 0;
        return false;
    }
    int data_size = *((int*)(data+ BUFF_SEPERATOR_LEN_BYTE)); // + 1 is to pass BUFF_SEPERATOR
    assert(data_size > 0 && data_size <= MAX_MSG_SIZE); // this should not happen unless
    // received data is corrupted somehow. In debug build we must fail
    // and try to debug. For fault tolerance purposes or when we are
    // using UDP and no checksumming is done we need to be prepared for
    // anything, hence the following check...
    if(data_size < 0 || data_size > MAX_MSG_SIZE) {
        // seems like we got rubbish in the buffer, lets skip 
        // to either the end of the buffer of to the beginning of 
        // a new message.
        int i = 1; // the first byte is already 
        // the separator, so we start looking from the second byte: i = 1
        while(i < size && data[i] != BUFF_SEPERATOR)
            i++;
        consumed_size = i;
        FILE_LOG(logWARNING)  << "MessageStream::do_produce():" << __LINE__ <<  " Warning: corrupted data was received. Discarded bytes: " << i
        		<< " data size: " << data_size;
        return false;
    }
    FILE_LOG(logDEBUG2) << "MessageStream::do_produce() message size (w/o header): " << data_size << " Header size: " << MSG_HEADER_SIZE;
    // not enough data was received to reconcile the message. Just return.
    if(MSG_HEADER_SIZE + data_size > size) {
    	FILE_LOG(logDEBUG2)  << "MessageStream::do_produce():" << __LINE__ <<  " : buffer is incomplete." <<
    			" Data required to reconcile message: " << MSG_HEADER_SIZE + data_size << ", data available: " << size;
        consumed_size = 0;
        return false;
    }
    data += MSG_HEADER_SIZE;
    if(!msg.ParseFromArray(data, data_size)) {
        // seems like we got rubbish in the buffer, lets skip 
        // to either the end of the buffer of to the beginning of 
        // a new message.
        int i = 0;
        while(i < size && data[i] != BUFF_SEPERATOR)
            i++;
        consumed_size = i + MSG_HEADER_SIZE;
        FILE_LOG(logWARNING) << "MessageStream::do_produce():" << __LINE__ <<  " Warning: corrupted data was received. Discarded bytes: " << i;
        return false;
    }
    consumed_size = data_size + MSG_HEADER_SIZE;
    return true;
}

bool MessageStream::produce(ManaMessageProtobuf& msg) {
    // if we already have something in the unconsumed buffer we need to consume
    // that first along with the rest of the incomplete message which is in the
    // new buffer. So we copy one message worth of data into the unconsumed
    // buffer and send that to do_consume.
    if(unconsumed_data_size_ != 0) {
    	FILE_LOG(logDEBUG2)  << "MessageStream:produce(): There's " << unconsumed_data_size_ << " Bytes of unconsumed data";
        int i = 0;
        while(i < new_data_size_ && unconsumed_data_size_ < MAX_MSG_SIZE && new_data_[i] != BUFF_SEPERATOR) {
            unconsumed_data_[unconsumed_data_size_++] = new_data_[i++];
        }
        // If the unconsumed_data_ buffer is full and there's still unconsumed data but the end of a message is
        // not reached then the received buffer was corrupted. We need to throw away all the unconsumed data.
        if(i < new_data_size_ && MAX_MSG_SIZE <= unconsumed_data_size_  && new_data_[i] != BUFF_SEPERATOR) {
        	assert(false);
        }
        assert(new_data_size_ - i >= 0);
        new_data_ += i;
        new_data_size_ -= i;
        // if that managed to extract a good message then return true
        // otherwise proceed to the rest of the buffer
        int consumed = 0;
        if(do_produce(unconsumed_data_, unconsumed_data_size_, msg, consumed)) {
            // assert: all the buffer must be consumed
            assert(consumed == unconsumed_data_size_);
            unconsumed_data_size_ = 0; // we consumed all of it.
            return true;
        }
        unconsumed_data_size_ -= consumed;
    }
    // new_data_size_ must never be a negative number
    assert(new_data_size_ >=0);

    // now all the unconsumed data from before is consumed. If no new data is
    // left, return false
    if(new_data_size_ <= 0)
        return false;

    // otherwise try reading and consuming the new data until either there's a success and
    // we consumed the whole buffer or ALMOST all of it. (i.e., until there's not enough data
    // to reconstruct a whole new message
    int consumed = 0;
    do {
        if(do_produce(new_data_, new_data_size_, msg, consumed)) {
            new_data_ += consumed;
            new_data_size_ -= consumed;
            assert(new_data_size_ >= 0);
            return true;
        }
        new_data_ += consumed;
        new_data_size_ -= consumed;
        assert(new_data_size_ >= 0);
    } while(consumed != 0 && new_data_size_ > 0);
 
    // if no data is left, return false
    if(new_data_size_ <= 0)
        return false;
    // if we get to here it means reading/parsing failed.
    // this means that we reached ALMOST the end of this buffer,
    // i.e., there's the beginning of a new message in new_buffer_
    // but not all of it (consumed == 0). In this case we copy the data
    // into unconsumed buffer for later use, i.e., when the rest of the message
    // arrives
    assert(consumed == 0);
    assert(new_data_[0] == BUFF_SEPERATOR);
    // there must not by any unconsumed data in unconsumed_data_.
    //assert(unconsumed_data_size_ == 0);
    int i = 0;
    while(new_data_size_ > 0) {
        unconsumed_data_[unconsumed_data_size_++] = new_data_[i++];
        new_data_size_--;
    }
    assert(new_data_size_ == 0); // now everything must be consumed or copied 
    // over to the other buffer.
    FILE_LOG(logDEBUG2)  << "MessageStream:produce(): " << unconsumed_data_size_ << " Bytes of data remained unconsumed." ;
    return false;
}

}
