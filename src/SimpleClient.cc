/*
 * @file SimpleClient.cc
 * @brief A simple base class based on which a publsher/subscriber class can be built.
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

#include "SimpleClient.h"
#include "ManaException.h"
#include "Utility.h"

using namespace std;

SimpleClient::SimpleClient(const string& str) : client_id_(str),
    broker_url_("ka:127.0.0.1:2350"), flag_session_established_(false) {}

SimpleClient::~SimpleClient() {}

void SimpleClient::handle_notification(const mana_message& m) {
    log_info("Application received notification: ");
    for(auto attr : m)
        log_info(attr.name().begin << " ");
}

void SimpleClient::start() {
    try {
        log_info("Starting client...");
        const string& url =  broker_url_;
        auto hndlr = std::bind(&SimpleClient::handle_notification, this, std::placeholders::_1);
        context_ = make_shared<mana::ManaContext>(client_id_, url, hndlr);
        context_->start();
        run();
        context_->join();
    } catch(mana::ManaException& e) {
        log_err("\n" << e.what() << "\n");
        exit(-1);
    }
}

void SimpleClient::stop() {
    context_->stop();
}

/*
 *  \brief A concrete implementation could implement this to do something
 *  useful, like subscribing or publishing. See details.
 *
 *  For instance we could do something like this in run()

    // subscribe to something:
    siena::int_t val = 5;
    mana_op_value* sov = new mana_op_value(siena::eq_id, val);
    siena::string_t cst = "const1";
    mana_filter fltr;
    fltr.add(cst, sov);
    context_->subscribe(fltr);

    //or publish something:
    mana_value* sv = new mana_value(static_cast<siena::int_t>(10));
    siena::string_t cst = "const1";
    mana_message msg;
    msg.add(cst, sv);
    context_->publish(msg);
 *
 */
void SimpleClient::run() {}

bool SimpleClient::set_broker(const string& str) {
    if(flag_session_established_) {
        log_err("Can not change broker after session is established");
        return false;
    }
    broker_url_ = str;
    return true;
}