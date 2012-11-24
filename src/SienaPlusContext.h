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
#include "ManaFwdTypes.h"

using namespace std;

namespace sienaplus {

class SienaPlusContext {
public:
	SienaPlusContext(const string&, const string& url, std::function<void(const mana_message&)>);
    SienaPlusContext(const SienaPlusContext& other) = delete; //disable copy constructor
	virtual ~SienaPlusContext();
	void publish(const string&);
	void publish(const mana_message&);
	void subscribe(const string& sub);
    void subscribe(const mana_filter&);
	void unsubscribe();
	void start();
	void stop();
	void join();
    bool session_established() const;
private:
	shared_ptr<NetworkConnector> net_connection_;
	boost::asio::io_service io_service_;
    // id of this connector
    string local_id_;
    // url of the remote broker
	string url_;
    // id of the remote node
    //unsigned int remote_id;
	bool connect();
	sienaplus::connection_type connection_type;
	std::function<void(const mana_message&)> app_notification_handler_;
	int port_;
	string address_;
	shared_ptr<boost::asio::io_service::work> work_;
	shared_ptr<thread> thread_;
	void receive_handler(NetworkConnector*, SienaPlusMessage&);
    void send_message(SienaPlusMessage&);
	bool is_connected();
	bool flag_has_subscription;
    bool flag_session_established_;
};

} /* namespace sienaplus */
#endif /* SIENAPLUSCONTEXT_H_ */
