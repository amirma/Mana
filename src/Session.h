/**
 * @file Session.h
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

#include <chrono>
#include <memory>
#include <boost/asio.hpp>
#include <siena/fwdtable.h>
#include "MessageReceiver.h"
#include "ManaMessage.pb.h"
#include "NetworkConnector.h"
#include "TCPNetworkConnector.h"
#include "UDPNetworkConnector.h"
#include "TaskScheduler.h"
#include "common.h"
#include "URL.h"
#include "Log.h"

using namespace std;

namespace mana {

/**
 * Session implementation
 *
 * @brief An instance of Session represents a client or a broker
 * with which there is an active session.
 *
 * The template T is the user of the class Session. An instance of of T
 * must implement the following methods:
 *
 *   \li void handle_session_termination(Session<T>& s);
 *
 *   Which is called when the session timesout (i.e., no heartbeat messages
 *   were received from the endpoint).
 *
 *   \li boost::asio::io_service& io_service();
 *
 *   This is called by the constructor of the Session. The io_service is used
 *   to provide a thread poll for the TaskScheduler instance that Session uses.
 *
 *   \li string& id()
 *
 *   Returns the identifier of the host.
 */

template <class T>
class Session {

public:

Session(const Session&) = delete;
Session& operator=(const Session&) = delete;

/**
 * @brief Constructor.
 * @param h User of the class. An instance of h must implement a set of callbacks which are mentioned in the
 * description of this class
 * @param in_nc  Pointer to the ingress network connector (i.e., the connector from which packets are received).
 * If this is null then a listener will be created listening on the parameter lo_url.
 * @param remote_id The identifier of the remote node
 * @param ifc The interface number in the forwarding table associated with this session
 * @param lo_url of the local node
 * @param rem_url of the remote node
 */
Session(T& h, const URL& lo_url, const URL& re_url, const string& id, siena::if_t ifc):
    host_(h), outgress_net_connector_(nullptr),
    remote_id_(id), local_url_(lo_url), remote_url_(re_url),
    remote_endpoint_(boost::asio::ip::address::from_string(remote_url_.address()), remote_url_.port()),
    iface_(ifc), flag_session_live_(false),
    task_scheduler_(h.io_service())  {

	if(remote_url_.protocol() == mana::connection_type::tcp) {
		outgress_net_connector_ = new TCPNetworkConnector<T>(h.io_service(), h, remote_url_);
	} else if(remote_url_.protocol() == mana::connection_type::ka) {
		outgress_net_connector_ = new TCPNetworkConnector<T>(h.io_service(), h, remote_url_);
		if(!connect())
			throw ManaException("Could not connect to " + remote_url_.url());
	} else if(remote_url_.protocol() == mana::connection_type::udp) {
		outgress_net_connector_ = new UDPNetworkConnector<T>(h.io_service(), h, remote_url_);
	} else {
		FILE_LOG(logERROR) << "Malformed URL or method not supported: " << remote_url_.url();
		throw ManaException("Malformed URL or method not supported: " + remote_url_.url());
	}

	establish();

    try {
        task_scheduler_.schedule_at_periods(std::bind(&Session<T>::check_session_liveness, this),
                DEFAULT_HEARTBEAT_INTERVAL_SECONDS, TimeUnit::second);
        unsigned int t = static_cast<unsigned int>((1 - DEFAULT_HEARTBEAT_SEND_INTERVAL_ADJ) * DEFAULT_HEARTBEAT_INTERVAL_SECONDS);
        task_scheduler_.schedule_at_periods(std::bind(&Session<T>::send_heartbeat, this), t, TimeUnit::second);
        update_hb_reception_ts();
    } catch(exception& e) {}

}

virtual ~Session() {
	if(outgress_net_connector_ != nullptr ) {
		delete outgress_net_connector_;
		outgress_net_connector_ = nullptr;
	}
}

/** @brief Get the interface associated with this session */
const siena::if_t& iface() const {
    return iface_;
}

/** @brief Get the identifier of the host at the other end of this session */
const string& remote_id() const {
    return remote_id_;
}

/** @brief If the session is active returns true */
bool is_active() const {
    return flag_session_live_;
}

const std::chrono::time_point<std::chrono::system_clock>& last_hb_reception_ts() const {
    return last_hb_reception_ts_;
}

void update_hb_reception_ts() {
    this->last_hb_reception_ts_ = std::chrono::system_clock::now();
    flag_session_live_ = false;
}

void send(ManaMessage const & msg) {
	//make sure all required fields are filled
	assert(msg.IsInitialized());
	assert(msg.has_type());
	if(outgress_net_connector_)
		outgress_net_connector_->send(msg);
}

void establish() {
	if(this->flag_session_live_)
		return;
	ManaMessage msg;
	// set sender id and type
	msg.set_sender(host_.id());
	msg.set_type(ManaMessage_message_type_t_START_SESSION);
	auto p = msg.mutable_key_value_map()->Add();
	p->set_key("url");
	p->set_value(local_url_.url());
	send(msg);
}

void terminate() {
	// FIXME:
	// send termination message
	this->outgress_net_connector_->disconnect();
}

private:

void send_heartbeat() {
    ManaMessage msg;
    // set sender id and type
    msg.set_sender(host_.id());
    msg.set_type(ManaMessage_message_type_t_HEARTBEAT);
    send(msg);
}

void check_session_liveness() {
	FILE_LOG(logDEBUG2) << "Session::check_neighbors_and_send_hb: checking neighbors...";
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    if(now - this->last_hb_reception_ts_ > std::chrono::seconds(DEFAULT_HEARTBEAT_INTERVAL_SECONDS)) {
        flag_session_live_ = false;
        host_.handle_session_termination(*this);
        //ingress_net_connector_->disconnect();
    }
}

bool connect() {
	if(outgress_net_connector_->is_connected())
		return true;
	if(!(outgress_net_connector_->connect(remote_url_))) {
		return false;
	}
	FILE_LOG(logDEBUG2) << "Session::connect(): Connected to " << remote_url_.url();
	return true;
}

// class properties
T& host_; // user of this instance. Callbacks are called on this instance
NetworkConnector<T>* outgress_net_connector_;
const string remote_id_;
const string local_id_;
const URL local_url_;
const URL remote_url_;
const boost::asio::ip::udp::endpoint remote_endpoint_;
const siena::if_t iface_; // interface id in the forwarding table
bool flag_session_live_; // true, if the session is active (based on HB messages)
std::chrono::time_point<std::chrono::system_clock> last_hb_reception_ts_; /* The
	time at which we last received a heartbeat from this neighbor */
TaskScheduler<std::function<void()>> task_scheduler_;
};

} /* namespace mana */

#endif /* NEIGHBORNODE_H_ */
