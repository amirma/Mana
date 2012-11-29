/*
 * @file SimpleClient.h
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

#ifndef SIMPLECLIENT_H_
#define SIMPLECLIENT_H_

#include <iostream>
#include "common.h"
#include <functional>
#include <memory>
#include <boost/algorithm/string.hpp>
#include "ManaContext.h"
#include "ManaFwdTypes.h"

using namespace std;

class SimpleClient {

public:
    SimpleClient(const string& str);
    virtual ~SimpleClient();
    SimpleClient(const SimpleClient&) = delete;
    SimpleClient& operator=(const SimpleClient&) = delete;
    void handle_notification(const mana_message& m);
    virtual void stop();
    virtual void start();
    virtual void run(); // the subclass implements this
    bool set_broker(const string& url);

protected:
    shared_ptr<mana::ManaContext> context_;
    string client_id_;
    string broker_url_;
    string local_url_; // url
    bool flag_session_established_;
    int listening_port_;

}; // class

#endif // SIMPLECLIENT_H_
