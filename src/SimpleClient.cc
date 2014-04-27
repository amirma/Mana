/*
 * @file SimpleClient.cc
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
#include "Utility.h"
#include "Mana.h"

using namespace std;

SimpleClient::SimpleClient(const string& str, const string& url, const string& broker) :
    client_id_(str), client_url_(url), broker_url_(broker) {}

SimpleClient::~SimpleClient() {}

void SimpleClient::handle_notification(const mana::ManaMessage& m) {
    cout << endl << "Application received notification: ";
    for(auto& attr : m)
    	cout << attr.name().begin << " ";
}

void SimpleClient::start() {
    try {
    	cout << endl << "Starting client...";
        auto hndlr = std::bind(&SimpleClient::handle_notification, this, std::placeholders::_1);
        context_ = make_shared<mana::ManaContext>(client_id_, client_url_, broker_url_, hndlr);
        context_->start();
        run();
        context_->join();
    } catch(mana::ManaException& e) {
    	cout << endl << e.what();
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
    ManaFilter fltr;
    fltr.add(cst, sov);
    context_->subscribe(fltr);

    //or publish something:
    mana_value* sv = new mana_value(static_cast<siena::int_t>(10));
    siena::string_t cst = "const1";
    ManaMessage msg;
    msg.add(cst, sv);
    context_->publish(msg);
 *
 */
void SimpleClient::run() {}
