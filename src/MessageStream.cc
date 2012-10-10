#include "MessageStream.h"

namespace sienaplus {

MessageStream::MessageStream() {
    unconsumed_data_size_ = 0;
    new_data_size_ = 0;
}

MessageStream::~MessageStream() {}

void MessageStream::consume(const unsigned char* buff, int size) {
    assert(new_data_size_ == 0); // there must not be any
    // data remained from before. If this assertion fails,
    // either the MessageStream::consume() is not called enough
    // times to consume all the available data in the previous new_data_
    // buffer. This can happen, for instance, when two separate threads,
    // access the same instance of MessageStream.
    new_data_ = buff;
    new_data_size_ = size;
    new_data_original_ = buff;
    new_data_original_size_ = size;
    log_debug("\nMessageStream:consume(): received new buffer. Buffer size: " << size);
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
bool MessageStream::do_produce(const unsigned char* data, int size, SienaPlusMessage& msg, int& consumed_size) const {
    assert(size > 0);
    assert(data[0] == BUFF_SEPERATOR);// this should not happen unless 
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
        log_warn("\nMessageStream::do_consume(): Warning: corrupted data was received. Discarded bytes: " << i);
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
    if( data_size < 0 || data_size > MAX_MSG_SIZE) {
        // seems like we got rubbish in the buffer, lets skip 
        // to either the end of the buffer of to the beginning of 
        // a new message.
        int i = 1; // the first byte is already 
        // the separator, so we start looking from the second byte: i = 1
        while(i < size && data[i] != BUFF_SEPERATOR)
            i++;
        consumed_size = i;
        log_warn("\nMessageStream::do_consume(): Warning: corrupted data was received. Discarded bytes: " << i
                << " data size: " << data_size);
        return false;
    }
    log_debug("\nMessageStream::consume(): message size: " << data_size);
    if(MSG_HEADER_SIZE + data_size > size) {
        log_debug("\nMessageStream::do_consume(): message is incomplete");
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
        log_warn("\nMessageStream::do_consume(): Warning: corrupted data was received. Discarded bytes: " << i);
        return false;
    }
    consumed_size = data_size + MSG_HEADER_SIZE;
    return true;
}

bool MessageStream::produce(SienaPlusMessage& msg) {
    // if we already have something in the unconsumed buffer we need to consume
    // that first along with the rest of the incomplete message which is in the
    // new buffer. So we copy one message worth of data into the unconsumed
    // buffer and send that to do_consume.
    if(unconsumed_data_size_ != 0) {
        log_debug("\nMessageStream:produce(): There's " << unconsumed_data_size_ << "bytes of remaining data");
        int i = 0;
        while(i < new_data_size_ && i < MAX_MSG_SIZE - unconsumed_data_size_ && new_data_[i] != BUFF_SEPERATOR) {
            unconsumed_data_[unconsumed_data_size_++] = new_data_[i++];
        }
        assert(new_data_size_ - i >= 0);
        new_data_ += i;
        new_data_size_ -= i;
        // if that managed to extract a good message the return true
        // otherwise proceed to the rest of the buffer
        int c = 0;
        if(do_produce(unconsumed_data_, unconsumed_data_size_ + i, msg, c)) {
            // assert: all the buffer must be consumed
            assert(c == unconsumed_data_size_);
            unconsumed_data_size_ = 0; // we consumed all of it.
            return true;
        }
        unconsumed_data_size_ = 0; // we consumed all of it.
    }
    assert(new_data_size_ >=0);

    // if no data left, return false
    if(new_data_size_ <= 0)
        return false;

    // try reading and consuming until either there's a success and 
    // or we consumed the whole buffer or ALMOST all of it.
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
    assert(unconsumed_data_size_ == 0);
    int i = 0;
    while(new_data_size_ > 0) {
        unconsumed_data_[i] = new_data_[i];
        new_data_size_--;
        i++;
    }
    assert(new_data_size_ == 0); // now everything must be consumed or copied 
    // over to the other buffer.
    unconsumed_data_size_ = i;
    return false;
}

}
