#ifndef MESSAGESTREAM_H_
#define MESSAGESTREAM_H_

#include "common.h"
#include "ManaMessage.pb.h"
#include <iostream>

namespace mana {

using namespace std;

class MessageStream {

    public:
        MessageStream();
        virtual ~MessageStream();
        MessageStream(const MessageStream&) = delete; // delete copy constructor
        void consume(const unsigned char* buff, int size);
        bool produce(ManaMessage& msg);
private:

    bool do_produce(const unsigned char*, int size, ManaMessage& msg, int& consumed) const;
    bool check_has_message_header();
    unsigned char unconsumed_data_[MAX_MSG_SIZE];
    int unconsumed_data_size_;
    const unsigned char* new_data_;
    int new_data_size_;
};

}
#endif
