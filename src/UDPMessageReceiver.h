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

template <class MessageReceiverClient>
class UDPMessageReceiver: public MessageReceiver<MessageReceiverClient> {
public:
	UDPMessageReceiver(boost::asio::io_service& srv, MessageReceiverClient& client_,
				const int port, const string& addr);
	virtual ~UDPMessageReceiver();
	void start();
	void stop();
};

} /* namespace mana */
#endif /* UDPMESSAGERECEIVER_H_ */
