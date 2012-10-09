#include "MessageStream.h"

namespace sienaplus {

MessageStream::MessageStream() {
    unconsumed_data_size_ = 0;
    new_data_size_ = 0;
}

void MessageStream::consume(const char* buff, int size) {
    new_data_ = buff;
    new_data_size_ = size;
    log_debug("\nMessageStream:consume(): received new buffer. Buffer size: " << size);
}

/*
 * Take a buffer 'data' with a maximum size of 'size' and deserialize into msg
 * and put the number of consumed bytes in consumed_size
 */
bool MessageStream::do_produce(const char* data, int size, SienaPlusMessage& msg, int& consumed_size) {
    assert(data[0] == BUFF_SEPERATOR);
    int data_size = *((int*)(data+ BUFF_SEPERATOR_LEN_BYTE)); // + 1 is to pass BUFF_SEPERATOR
    assert(data_size <= MAX_MSG_SIZE);
    log_debug("\nMessageStream::consume(): message size: " << data_size);
    data += MSG_HEADER_SIZE;
    if(MSG_HEADER_SIZE + data_size > size) {
        log_debug("\nMessageStream::do_consume(): message is incomplete");
        consumed_size = 0;
        return false;
    }
    if(!msg.ParseFromArray(data, data_size)) {
        log_warn("\nMessageStream::do_consume(): Could not parse buffer. Message size: " << data_size);
        consumed_size = data_size + MSG_HEADER_SIZE;
        return false;
    }
    consumed_size = data_size + MSG_HEADER_SIZE;
    return true;
}

bool MessageStream::produce(SienaPlusMessage& msg) {
    // if we already have something in the unconsumed buffer
    // we need to consume that first along with the rest of the 
    // incomplete message which is in the new buffer. So
    // we copy one message worth of data into the unconsumed buffer
    // and send that to do_consume.
    if(unconsumed_data_size_ != 0) {
        log_debug("\nMessageStream:produce(): There's " << unconsumed_data_size_ << "bytes of remaining data");
        int i = 0;
        while(i < MAX_MSG_SIZE - unconsumed_data_size_ && new_data_[i] != BUFF_SEPERATOR) {
            unconsumed_data_[unconsumed_data_size_++] = new_data_[i++];
        }
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
            return true;
        }
        new_data_ += consumed;
        new_data_size_ -= consumed;
    } while(consumed != 0 && new_data_size_ > 0);
 
    // if no data left, return false
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
    int i = 0;
    while(new_data_size_ > 0) {
        unconsumed_data_[i] = new_data_[i];
        new_data_size_--;
        i++;
    }
    unconsumed_data_size_ = i;
    return false;
}

}
