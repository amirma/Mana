/*
 * COMMON.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef COMMON_MANA_H_
#define COMMON_MANA_H_

#include <string>
#include <mutex>
#include <iomanip>
#include <ctime>


// utility macros
#define is_in_container(container, key) (container.find(key)!=container.end())

// concurrency macros
/*  The following two macros are based on Herb Sutter's template at
 *  http://www.drdobbs.com/windows/associate-mutexes-with-data-to-prevent-r/224701827?pgno=3
 *  */
#define PROTECTED_WITH(MutType)  \
     public:    void    lock()            {  mut_.lock();  } \
     public:    bool     try_lock()    {  return mut_.try_lock();  } \
     public:    void     unlock()      {  mut_.unlock();  } \
     private:   MutType mut_;

#define PROTECTED_MEMBER(Type,name) \
     public:    Type&    name()     { assert(!mut_.try_lock()); return name##_; } \
     public:    const Type&    const_##name()     { return name##_; } \
     private:   Type    name##_;


// global constants
#define BUFF_SEPERATOR 254 //ETB
#define BUFF_SEPERATOR_LEN_BYTE 1
#define MAX_MSG_SIZE  3000000 //Bytes
#define MAX_PCKT_SIZE 1400 //Bytes

namespace mana {

enum enum_connection_type {
    ka,
    tcp,
    udp
};
typedef enum_connection_type connection_type;
typedef unsigned char byte;

/*
 * These function convert from/to  a ManaMessageProtobuf to different
 * Mana types. Note that when filling in a protobuf the field
 * 'sender' is not set in these function.
 */

class ManaMessageProtobuf;
class ManaMessage;
class ManaFilter;


void to_ManaMessage(const ManaMessageProtobuf& buff, ManaMessage& msg);
void to_ManaFilter(const ManaMessageProtobuf& buff, ManaFilter& pred);
//
void to_protobuf(const ManaMessage& msg, ManaMessageProtobuf& buff);
void to_protobuf(const ManaFilter& predg, ManaMessageProtobuf& buff);

const int MSG_HEADER_SIZE = sizeof(int) + BUFF_SEPERATOR_LEN_BYTE;

const unsigned int DEFAULT_HEARTBEAT_INTERVAL_SECONDS = 5; // session liveness
// is checked every DEFAULT_HEARTBEAT_INTERVAL_SECONDS time units
const float DEFAULT_HEARTBEAT_SEND_INTERVAL_ADJ = 0.25f; // heart beat messages
// are sent every DEFAULT_HEARTBEAT_INTERVAL_SECONDS * (1-DEFAULT_HEARTBEAT_INTERVAL_ADJ)
// time units
}

#endif /* COMMON_H_ */
