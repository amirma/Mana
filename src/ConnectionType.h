/*
 * ConnectionType.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef CONNECTIONTYPE_H_
#define CONNECTIONTYPE_H_

namespace sienaplus {

	enum enum_connection_type {
		ka,
		tcp,
		udp
	};

	typedef enum_connection_type connection_type;
}
#endif /* CONNECTIONTYPE_H_ */
