/**
 * @file Session.cc
 * Session implementation
 *
 * @brief An instance of Session represents a client or a broker
 * with which there is an active session.
 *
 * The template T is the user of the class Session. An instance of of T
 * must implement the following methods:
 *
 *   void handle_session_termination(Session<T>& s);
 *   Which is called when the session timesout (i.e., no heartbeat messages
 *   were received from the endpoint).
 *
 *   boost::asio::io_service& io_service();
 *   This is called by the constructor of the Session. The io_service is used
 *   to provide a thread poll for the TaskScheduler instance that Session uses.
 *
 *   string& id()
 *   Returns the identifier of the host.
 *
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

#ifndef NEIGHBORNODE_H_
#define NEIGHBORNODE_H_

#include <queue>
#include <array>
#include <mutex>
#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include <siena/fwdtable.h>
#include "MessageReceiver.h"
#include "ManaMessage.pb.h"
#include "NetworkConnector.h"
#include "TaskScheduler.h"
#include "common.h"

using namespace std;

namespace mana {

template <class T>
class Session {

public:

Session(const Session&) = delete;
Session& operator=(const Session&) = delete;

Session(T& h, NetworkConnector<T>* nc, const string& id, siena::if_t ifc) :
    host_(h), net_connector_(nc), remote_id_(id), iface_(ifc), flag_is_active_(false),
    task_scheduler_(h.io_service())  {
    try {
        task_scheduler_.schedule_at_periods(std::bind(&Session<T>::check_session_liveness, this),
                DEFAULT_HEARTBEAT_INTERVAL_SECONDS, TimeUnit::second);
        unsigned int t = static_cast<unsigned int>((1 - DEFAULT_HEARTBEAT_SEND_INTERVAL_ADJ) * DEFAULT_HEARTBEAT_INTERVAL_SECONDS);
        task_scheduler_.schedule_at_periods(std::bind(&Session<T>::send_heartbeat, this), t, TimeUnit::second);
        update_hb_reception_ts();
    } catch(exception& e) {}
}

virtual ~Session() {}

NetworkConnector<T>* net_connector() {
    return net_connector_;
}

/** @brief Get the interface assciated with this session */
const siena::if_t& iface() const {
    return iface_;
}

/** @brief Get the identifier of the host at the other end of this session */
const string& remote_id() const {
    return remote_id_;
}

/** @brief If the session is active returns true */
bool is_active() const {
    return flag_is_active_;
}

const std::chrono::time_point<std::chrono::system_clock>& last_hb_reception_ts() const {
    return last_hb_reception_ts_;
}

void update_hb_reception_ts() {
    this->last_hb_reception_ts_ = std::chrono::system_clock::now();
    flag_is_active_ = false;
}

private:

void send_heartbeat() {
    ManaMessage msg;
    // set sender id and type
    msg.set_sender(host_.id());
    msg.set_type(ManaMessage_message_type_t_HEARTBEAT);
    // fill in the constraints : type, name, operator, value
    //make sure all required fields are filled
    assert(msg.IsInitialized());
    assert(msg.has_type());
    send_message(msg);
}

void check_session_liveness() {
    log_info("Session::check_neighbors_and_send_hb: checking neighbors...");
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    if(now - this->last_hb_reception_ts_ > std::chrono::seconds(DEFAULT_HEARTBEAT_INTERVAL_SECONDS)) {
        flag_is_active_ = false;
        host_.handle_session_termination(*this);
        net_connector_->disconnect();
    }
}


void send_message(ManaMessage& msg) {
	net_connector_->send(msg);
}

// class properties
T& host_;
NetworkConnector<T>* net_connector_;
string remote_id_;
siena::if_t iface_;
bool flag_is_active_;
/* The time at which we last received a heartbeat from this neighbor */
std::chrono::time_point<std::chrono::system_clock> last_hb_reception_ts_;
TaskScheduler<std::function<void()>> task_scheduler_;
};


} /* namespace mana */

#endif /* NEIGHBORNODE_H_ */
