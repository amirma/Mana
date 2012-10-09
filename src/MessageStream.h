#ifndef MESSAGESTREAM_H_
#define MESSAGESTREAM_H_

#include "common.h"
#include "SienaPlusMessage.pb.h"
#include <iostream>

namespace sienaplus {

using namespace std;

class MessageStream {

    public:
        MessageStream();
        
        void consume(const char* buff, int size);
        bool produce(SienaPlusMessage& msg);
private:

    bool do_produce(const char*, int size, SienaPlusMessage& msg, int& consumed);
    bool check_has_message_header();
    char unconsumed_data_[MAX_MSG_SIZE];
    int unconsumed_data_size_;
    const char* new_data_;
    int new_data_size_;
};

}
#endif
