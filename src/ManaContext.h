/**
* @file ManaContext.h
*
* @author Amir Malekpour
* @version 0.1
*
* Copyright © 2012 Amir Malekpour
*
*  Mana is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  Mana is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*  FOR A PARTICULAR PURPOSE. For more details see the GNU General Public License
*  at <http: *www.gnu.org/licenses/>
*/

#ifndef SIENAPLUSCONTEXT_H_
#define SIENAPLUSCONTEXT_H_

#include <functional>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include "MessageSender.h"
#include "TCPMessageSender.h"
#include "common.h"
#include "ManaMessage.pb.h"
#include "ManaFwdTypes.h"
#include "TaskScheduler.h"
#include "Session.h"
#include "URL.h"

using namespace std;

namespace mana {

/**
* @brief The client's interface to the publish/subscribe network.
*
* Every client instantiates a instance of this class to connect to a broker and
* subscribe to messages or publish messages. As an example see {@link SimpleClient.cc}
*/
class ManaContext {
public:
	/** Constructor
	 * @param id The client's identifier
	 * @param loc_url The clients local url
	 * @param rem_url The URL of the remote host (broker)
	 * @param h A function object which is called when a message match occurs. The signiture of
	 * the function object is as <b>void functor(const mana_message& msg)</b>
	 */
    ManaContext(const string& id, const string& loc_url, const string& rem_url, std::function<void(const mana_message&)> h);
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
    void handle_message(ManaMessage& msg, MessageReceiver<ManaContext>* mr);
    void handle_session_termination(Session<ManaContext>& s);
    boost::asio::io_service& io_service();
    const string& id() const;
private:
    boost::asio::io_service io_service_;
    //shared_ptr<MessageSender<ManaContext> > net_connection_;
    shared_ptr<MessageReceiver<ManaContext>> message_receiver_;
    // id of this connector
    string local_id_;
    // URL of the remote broker
    URL local_url_;
    URL remote_url_;
    std::function<void(const mana_message&)> app_notification_handler_;
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
