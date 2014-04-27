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
#include "ManaMessageProtobuf.pb.h"
#include "MessageSender.h"
#include "TCPMessageSender.h"
#include "UDPMessageSender.h"
#include "TaskScheduler.h"
#include "common.h"
#include "URL.h"
#include "Log.h"
#include "StateMachine.h"

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
    iface_(ifc), flg_session_live_(false),
    task_scheduler_(h.io_service()), state_machine_(3)  {

	setup_state_machine();

	if(remote_url_.protocol() == mana::connection_type::tcp) {
		outgress_net_connector_ = new TCPMessageSender<T>(h.io_service(), h, remote_url_);
	} else if(remote_url_.protocol() == mana::connection_type::ka) {
		outgress_net_connector_ = new TCPMessageSender<T>(h.io_service(), h, remote_url_);
		if(!connect())
			throw ManaException("Could not connect to " + remote_url_.url());
	} else if(remote_url_.protocol() == mana::connection_type::udp) {
		outgress_net_connector_ = new UDPMessageSender<T>(h.io_service(), h, remote_url_);
	} else {
		FILE_LOG(logERROR) << "Malformed URL or method not supported: " << remote_url_.url();
		throw ManaException("Malformed URL or method not supported: " + remote_url_.url());
	}

	//establish();

    try {
        task_scheduler_.schedule_at_periods(std::bind(&Session<T>::check_session_liveness, this),
                DEFAULT_HEARTBEAT_INTERVAL_SECONDS, TimeUnit::second);
        unsigned int t = static_cast<unsigned int>((1 - DEFAULT_HEARTBEAT_SEND_INTERVAL_ADJ) * DEFAULT_HEARTBEAT_INTERVAL_SECONDS);
        task_scheduler_.schedule_at_periods(std::bind(&Session<T>::send_heartbeat, this), t, TimeUnit::second);
        //
        update_hb_reception_ts();
    } catch(const exception& e) {}

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
    return flg_session_live_;
}

void send(ManaMessageProtobuf const & msg) {
    assert(msg.IsInitialized());
    assert(msg.has_type());
    outgress_net_connector_->send(msg);
}

void establish() {
    if(this->is_active())
    	return;
    ManaMessageProtobuf msg;
    // set sender id and type
    msg.set_sender(host_.id());
    msg.set_type(ManaMessageProtobuf_message_type_t_START_SESSION);
    auto p = msg.mutable_key_value_map()->Add();
    p->set_key("url");
    p->set_value(local_url_.url());
    send(msg);
    assert(state_machine_.current_state() == NOT_ESTABLISHED);
    //state_machine_.process(REQ_SENT);
}

void terminate() {
    // FIXME:
    // send termination message
    this->outgress_net_connector_->disconnect();
}

void handle_session_msg(const ManaMessageProtobuf& msg) {
	switch(msg.type()) {
	case ManaMessageProtobuf_message_type_t_HEARTBEAT :
		update_hb_reception_ts();
		FILE_LOG(logDEBUG2) << "Session::handle_session_msg: Received heartbeat from " << msg.sender();
		break;
	case ManaMessageProtobuf_message_type_t_START_SESSION:
    case ManaMessageProtobuf_message_type_t_START_SESSION_ACK :
    case ManaMessageProtobuf_message_type_t_START_SESSION_ACK_ACK:
    case ManaMessageProtobuf_message_type_t_TERMINATE_SESSION :
    case ManaMessageProtobuf_message_type_t_TERMINATE_SESSION_ACK :
    	//session_.handle_sesstion_message(msg);
    	break;
	default:
		FILE_LOG(logINFO) << "Session::handle_session_msg: unrecognized message type.";
		break;
	}
}

private:

void update_hb_reception_ts() {
    this->last_hb_reception_ts_ = std::chrono::system_clock::now();
    flg_session_live_ = false;
}

const std::chrono::time_point<std::chrono::system_clock>& last_hb_reception_ts() const {
    return last_hb_reception_ts_;
}

void send_heartbeat() {
    ManaMessageProtobuf msg;
    // set sender id and type
    msg.set_sender(host_.id());
    msg.set_type(ManaMessageProtobuf_message_type_t_HEARTBEAT);
    send(msg);
}

void check_session_liveness() {
	FILE_LOG(logDEBUG2) << "Session::check_neighbors_and_send_hb: checking neighbors...";
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    if(now - this->last_hb_reception_ts_ > std::chrono::seconds(DEFAULT_HEARTBEAT_INTERVAL_SECONDS)) {
        flg_session_live_ = false;
        host_.handle_session_termination(*this);
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

void setup_state_machine() {
	// from not established goes to request sent with input REQ_SENT
	state_machine_.add_transition(NOT_ESTABLISHED, REQ_SENT, REQ_SENT);
	state_machine_.add_transition(REQ_SENT, ESTABLISHED, ESTABLISHED);
	state_machine_.add_transition(ESTABLISHED, TERM_REQ_SENT, TERM_REQ_SENT);
	//state_machine_.add_transition(TERM_REQ_SENT, NOT_ESTABLISHED, NOT_ESTABLISHED);

	// specify blocking state transitions
	//state_machine_.set_blocking(REQ_SENT);
	//state_machine_.set_blocking(TERM_REQ_SENT);
	// now add transition handlers*/
}

// class properties
T& host_; // user of this instance. Callbacks are called on this instance
MessageSender<T>* outgress_net_connector_;
const string remote_id_;
const string local_id_;
const URL local_url_;
const URL remote_url_;
const boost::asio::ip::udp::endpoint remote_endpoint_;
const siena::if_t iface_; // interface id in the forwarding table
bool flg_session_live_; // true, if the session is active (based on HB messages)
std::chrono::time_point<std::chrono::system_clock> last_hb_reception_ts_; /* The
	time at which we last received a heartbeat from this neighbor */
TaskScheduler<std::function<void()>> task_scheduler_;

enum States {NOT_ESTABLISHED, REQ_SENT, ESTABLISHED, TERM_REQ_SENT};
StateMachine state_machine_;

};

} /* namespace mana */

#endif /* NEIGHBORNODE_H_ */
