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

namespace mana {

class UDPMessageReceiver: public mana::MessageReceiver {
public:
	UDPMessageReceiver(boost::asio::io_service&, const int, const string&, const std::function<void(NetworkConnector*, ManaMessage&)>&);
	virtual ~UDPMessageReceiver();
	void start();
	void stop();
};

} /* namespace mana */
#endif /* UDPMESSAGERECEIVER_H_ */
