/*
 * ManaContext.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef SIENAPLUSCONTEXT_H_
#define SIENAPLUSCONTEXT_H_

#include <functional>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include "NetworkConnector.h"
#include "TCPNetworkConnector.h"
#include "common.h"
#include "ManaMessage.pb.h"
#include "ManaFwdTypes.h"
#include "TaskScheduler.h"
#include "Session.h"

using namespace std;

namespace mana {

class ManaContext {
public:
    ManaContext(const string&, const string& url, std::function<void(const mana_message&)>);
    ManaContext(const ManaContext& other) = delete; //disable copy constructor
    virtual ~ManaContext();
    void publish(const string&);
    void publish(const mana_message&);
    void subscribe(const string& sub);
    void subscribe(const mana_filter&);
    void unsubscribe();
    void start();
    void stop();
    void join();
    bool session_established() const;
    void handle_message(NetworkConnector<ManaContext>& nc, ManaMessage& buff);
    void handle_session_termination(Session<ManaContext>& s);
    boost::asio::io_service& io_service();
    const string& id() const;
private:
    boost::asio::io_service io_service_;
    shared_ptr<NetworkConnector<ManaContext> > net_connection_;
    // id of this connector
    string local_id_;
    // url of the remote broker
    string remote_node_url_;
    // id of the remote node
    //unsigned int remote_id;
    bool connect();
    mana::connection_type connection_type;
    std::function<void(const mana_message&)> app_notification_handler_;
    int port_;
    string address_;
    shared_ptr<boost::asio::io_service::work> work_;
    shared_ptr<thread> thread_;
    void send_message(ManaMessage&);
    bool is_connected();
    bool flag_has_subscription;
    bool flag_session_established_;
    TaskScheduler<std::function<void()>> task_scheduler_;
    shared_ptr<Session<ManaContext>> session_;
};

} /* namespace mana */
#endif /* SIENAPLUSCONTEXT_H_ */
