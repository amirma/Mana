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
#include <siena/types.h>
#include "ManaMessage.pb.h"
#include "ManaFwdTypes.h"


// utility macros
#define is_in_container(container, key) (container.find(key)!=container.end())


// logging macros
#define FLG_PR_DEBUG false
#define FLG_PR_WARN  true


#define log_err(txt) do { \
                    std::time_t t = std::time(nullptr); \
                    char str[100];\
                    std::strftime(str, 100, "%F %T", std::localtime(&t));\
                    cout << endl << str << " " <<  txt;\
                    cout.flush();\
                    } while(false);


#define log_warn(txt) while(FLG_PR_WARN) { \
                    std::time_t t = std::time(nullptr); \
                    char str[100];\
                    std::strftime(str, 100, "%F %T", std::localtime(&t));\
                    cout << endl << str << " " <<  txt;\
                    cout.flush();\
                    break; \
                    };

#define log_debug(txt) while(FLG_PR_DEBUG) { \
                    std::time_t t = std::time(nullptr); \
                    char str[100];\
                    std::strftime(str, 100, "%F %T", std::localtime(&t));\
                    cout << endl << str << " " <<  txt;\
                    cout.flush();\
                    break; \
                    };

#define log_info(txt) do { \
                    std::time_t t = std::time(nullptr); \
                    char str[100];\
                    std::strftime(str, 100, "%F %T", std::localtime(&t));\
                    cout << endl << str << " " <<  txt;\
                    cout.flush();\
                    } while(false);

// concurrency macros
/*  The following two macros are based on Herb Sutter's template at
 *  http://www.drdobbs.com/windows/associate-mutexes-with-data-to-prevent-r/224701827?pgno=3
 *  */
#define PROTECTED_WITH(MutType)  \
     public:    void    lock()            {  mut_.lock();  } \
     public:    bool     try_lock()    {  return mut_.try_lock();  } \
     public:    void     unlock()      {  mut_.unlock();  } \
     private:   MutType mut_;

#define PROTECTED_MEMBER(Type,name)\
     public:    Type&    name()     { assert(!mut_.try_lock()); return name##_; } \
     public:    const Type&    const_##name()     { return name##_; } \
     private:   Type    name##_;


// constants
#define BUFF_SEPERATOR 23 //ETB
#define BUFF_SEPERATOR_LEN_BYTE 1
#define MAX_MSG_SIZE 9000 //Bytes

using namespace mana;

namespace mana {

enum enum_connection_type {
    ka,
    tcp,
    udp
};
typedef enum_connection_type connection_type;

/*
 * These function convert from/to  a SienaMessagePlus to different
 * siena types. Note that when filling in a protobuf the field
 * 'sender' is not set in these function.
 */
void to_mana_message(const ManaMessage& buff, mana_message& msg);
void to_mana_filter(const ManaMessage& buff, mana_filter& pred);
//
void to_protobuf(const mana_message& msg, ManaMessage& buff);
void to_protobuf(const mana_filter& predg, ManaMessage& buff);

const int MSG_HEADER_SIZE = sizeof(int) + BUFF_SEPERATOR_LEN_BYTE;

const unsigned int DEFAULT_NUM_OF_BROKER_THREADS  = 1;
const unsigned int DEFAULT_HEARTBEAT_INTERVAL_SECONDS = 500; // session liveness
// is checked every DEFAULT_HEARTBEAT_INTERVAL_SECONDS time units
const float DEFAULT_HEARTBEAT_SEND_INTERVAL_ADJ = 0.25f; // heart beat messages
// are sent every DEFAULT_HEARTBEAT_INTERVAL_SECONDS * (1-DEFAULT_HEARTBEAT_INTERVAL_ADJ)
// time units
}

#endif /* COMMON_H_ */
