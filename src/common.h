/*
 * COMMON.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <siena/types.h>
#include "SienaPlusMessage.pb.h"
#include "ManaFwdTypes.h"
#include <string>
#include <mutex>


// utility macros
#define is_in_container(container, key) container.find(key)!=container.end()


// logging macros
#define FLG_PR_DEBUG false
#define FLG_PR_WARN  true


#define log_err(txt) do { \
                    cout << txt;\
                    cout.flush();\
                    } while(false);


#define log_warn(txt) while(FLG_PR_WARN) { \
                    cout << txt;\
                    cout.flush();\
                    break; \
                    };

#define log_debug(txt) while(FLG_PR_DEBUG) { \
                    cout << txt;\
                    cout.flush();\
                    break; \
                    };

#define log_info(txt) do { \
                    cout << txt;\
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

namespace sienaplus {

	enum enum_connection_type {
		ka,
		tcp,
		udp
	};
	typedef enum_connection_type connection_type;

/*
 * These function conver from/to  a SienaMessagePlus to different
 * siena types. Note that when filling in a protobuf the field
 * 'sender' is not set in these function.
 */
    void to_mana_message(const SienaPlusMessage& buff, mana_message& msg);
    void to_mana_filter(const SienaPlusMessage& buff, mana_filter& pred);
    //
    void to_protobuf(const mana_message& msg, SienaPlusMessage& buff);
    void to_protobuf(const mana_filter& predg, SienaPlusMessage& buff);

const int MSG_HEADER_SIZE = sizeof(int) + BUFF_SEPERATOR_LEN_BYTE;
};

#endif /* COMMON_H_ */
