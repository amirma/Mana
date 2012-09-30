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

namespace sienaplus {

	enum enum_connection_type {
		ka,
		tcp,
		udp
	};

	typedef enum_connection_type connection_type;

	const int MAX_MSG_SIZE = 9000; //Bytes

};

#endif /* COMMON_H_ */
