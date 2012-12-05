/*
 * @file Session.cc
 * Session implementation
 *
 * @brief An instance of Session represents a client or a broker
 * with which there is an active session.
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

#include <array>
#include <queue>
#include <mutex>
#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include <siena/fwdtable.h>
#include "MessageReceiver.h"
#include "ManaMessage.pb.h"
#include "NetworkConnector.h"
#include "TaskScheduler.h"

using namespace std;

namespace mana {

template <class T>
class Session {
public:

Session(const Session&) = delete;
Session& operator=(const Session&) = delete;

Session(T& h, NetworkConnector<T>* nc, const string& id, siena::if_t ifc):
	host_(h), net_connector_(nc), id_(id), iface_(ifc), flag_is_active_(false) {}

virtual ~Session() {}

NetworkConnector<T>* net_connector() {
    return net_connector_;
}

/** @brief Get the interface assciated with this session */
const siena::if_t& iface() const {
    return iface_;
}

/** @brief Get the identifier of the host at the other end of this session */
const string& id() const {
    return id_;
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
}

private:

void check_session_liveness() {
    log_info("\nBroker::check_neighbors_and_send_hb: checking neighbors...");
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    if(now - this->last_hb_reception_ts_ > std::chrono::seconds(DEFAULT_HEARTBEAT_INTERVAL_SECONDS)) {
        log_info("\nBroker::check_neighbor_and_send_hb: removing neighbor " << id_);
        net_connector_->disconnect();
    }
}

    // class properties
    T& host_;
    NetworkConnector<T>* net_connector_;
    string id_;
    siena::if_t iface_;
    bool flag_is_active_;
    /* The time at which we last received a heartbeat from this neighbor */
    std::chrono::time_point<std::chrono::system_clock> last_hb_reception_ts_;
};


} /* namespace mana */

#endif /* NEIGHBORNODE_H_ */
