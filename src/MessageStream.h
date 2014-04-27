#ifndef MESSAGESTREAM_H_
#define MESSAGESTREAM_H_

#include "common.h"
#include "ManaMessageProtobuf.pb.h"

namespace mana {

using namespace std;

class MessageStream {

    public:
        MessageStream();
        virtual ~MessageStream();
        MessageStream(const MessageStream&) = delete; // delete copy constructor
        void consume(const byte* buff, int size);
        bool produce(ManaMessageProtobuf& msg);
private:

    bool do_produce(const byte*, int size, ManaMessageProtobuf& msg, int& consumed) const;
    bool check_has_message_header();
    byte unconsumed_data_[MAX_MSG_SIZE];
    int unconsumed_data_size_;
    const byte* new_data_;
    int new_data_size_;
};

}
#endif
