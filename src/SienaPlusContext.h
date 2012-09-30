/*
 * SienaPlusContext.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef SIENAPLUSCONTEXT_H_
#define SIENAPLUSCONTEXT_H_

#include <functional>
#include <memory>
#include "NetworkConnector.h"
#include <boost/asio.hpp>
#include "common.h"
#include <thread>
#include "SienaPlusMessage.pb.h"
#include "sff.bzr/simple_fwd_types.h"

using namespace std;

namespace sienaplus {

class SienaPlusContext {
public:
	SienaPlusContext(const string& url, std::function<void(const simple_message&)>);
	virtual ~SienaPlusContext();
	void publish();
	void subscribe(const string& sub);
    void subscribe(const simple_filter&);
	void unsubscribe();
	void start();
	void stop();
	bool is_connected();
	void join();
private:
	shared_ptr<NetworkConnector> net_connection_;
	boost::asio::io_service io_service_;
	bool flag_has_subscription;
	string url_;
	bool connect();
	sienaplus::connection_type connection_type;
	std::function<void(const simple_message&)> app_notification_handler_;
	int port_;
	string address_;
	shared_ptr<boost::asio::io_service::work> work_;
	shared_ptr<thread> thread_;
	void receive_handler(const char*, int);

};

} /* namespace sienaplus */
#endif /* SIENAPLUSCONTEXT_H_ */
