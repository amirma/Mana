/*
 * UDPMessageReceiver.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef UDPMESSAGERECEIVER_H_
#define UDPMESSAGERECEIVER_H_

#include "MessageReceiver.h"
#include <functional>
#include <string>

using namespace std;

namespace sienaplus {

class UDPMessageReceiver: public sienaplus::MessageReceiver {
public:
	UDPMessageReceiver(boost::asio::io_service&, const int, const string&, const std::function<void(NetworkConnector*, const char*, int)>&);
	virtual ~UDPMessageReceiver();
	void start();
	void stop();
};

} /* namespace sienaplus */
#endif /* UDPMESSAGERECEIVER_H_ */
