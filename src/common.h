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
#include "sff.bzr/simple_fwd_types.h"

#define FLG_PR_DEBUG false

#define is_in_container(container, key) container.find(key)!=container.end()
#define log_debug(txt) while(FLG_PR_DEBUG) { \
                    cout << endl << txt;\
                    cout.flush();\
                    break; \
                    };

#define log_info(txt) do { \
                    cout << txt;\
                    cout.flush();\
                    } while(false);

#define log_err(txt) do { \
                    cout << endl << txt;\
                    cout.flush();\
                    } while(false);

namespace sienaplus {

	enum enum_connection_type {
		ka,
		tcp,
		udp
	};

	typedef enum_connection_type connection_type;

	const int MAX_MSG_SIZE = 9000; //Bytes

/*
 * These function conver from/to  a SienaMessagePlus to different 
 * siena types. Note that when filling in a protobuf the field 
 * 'sender' is not set in these function. 
 */
    void to_simple_message(const SienaPlusMessage& buff, simple_message& msg);
    void to_simple_filter(const SienaPlusMessage& buff, simple_filter& pred);
    //
    void to_protobuf(const simple_message& msg, SienaPlusMessage& buff);
    void to_protobuf(const simple_filter& predg, SienaPlusMessage& buff);

};

#endif /* COMMON_H_ */
