/*
 * @file Broker.h
 * Broker implementation
 *
 * @brief This class implements a messagin broker.
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

#ifndef BROKER_H_
#define BROKER_H_

#include <queue>
#include <mutex>
#include <chrono>
#include <memory>
#include <array>
#include <boost/asio.hpp>
#include <siena/fwdtable.h>
#include "MessageReceiver.h"
#include "ManaMessage.pb.h"
#include "NetworkConnector.h"
#include "TCPNetworkConnector.h"
#include "TaskScheduler.h"
#include "Session.h"

#define DEFAULT_NUM_OF_THREADS 1

using namespace std;

namespace mana {

/**  \brief This class generates interface numbers and maintains
*   a list of interface numbers that are freed.
*/
class IFaceNoGenerator {
    public:
        IFaceNoGenerator() {
            last_ = 0;
        }

        ~IFaceNoGenerator(){}

        siena::if_t borrow_number() {
            lock_guard<std::mutex> lock_(mutex_);
            if(queue_.size() == 0)
                return ++last_;
            siena::if_t r = queue_.front();
            queue_.pop();
            return r;
        }

        void free_number(siena::if_t n) {
            lock_guard<std::mutex> lock_(mutex_);
            queue_.push(n);
        }

    private:
        siena::if_t last_;
        queue<siena::if_t> queue_;
        mutex mutex_;
};

// forward declaration so that we can have a pointer to
// the broker in BrokerMatchMessageHandler
class BrokerMatchMessageHandler;

class Broker {
    // we put a shared data with its associated
    // mutex in one place, for easy management and less bugs.
    struct FwdTableWrapper {
        PROTECTED_WITH(std::mutex);
        PROTECTED_MEMBER(siena::FwdTable, fwd_table);
    };

public:
    Broker(const string& id);
    Broker(const Broker&) = delete; //disable copy constructor
    virtual ~Broker();
    void start();
    void shutdown();
    boost::asio::io_service& io_service();

    // Use this method to add more transport protocols to the broker
    void add_transport(string);
    void handle_message(ManaMessage&);
    void handle_sub(NetworkConnector<Broker>&, ManaMessage&);
    void handle_not(ManaMessage&);
    void handle_heartbeat(ManaMessage&);
    void handle_message(NetworkConnector<Broker>& nc, ManaMessage& msg);
    void handle_connect(shared_ptr<NetworkConnector<Broker>>& c);
    // we want BrokerMatchMessageHandler to be able to call
    // the private method 'handle_match'
    friend class BrokerMatchMessageHandler;
private:

    // private methods.
    void check_neighbors_and_send_hb();
    bool handle_match(siena::if_t, const siena::message&);
    // class properties
    boost::asio::io_service io_service_;
    vector<shared_ptr<MessageReceiver<Broker>>> message_receivers;
    FwdTableWrapper fwd_table_wrapper_; // the main forwarding table
    IFaceNoGenerator iface_no_generator_; /* a number generator for generating unique numbers to represent
     clients/neighbors */
    map<string, shared_ptr<Session<Broker> >> neighbors_by_id_; /* map interface/client/neighbors
    to the connections */
    map<siena::if_t, shared_ptr<Session<Broker> >> neighbors_by_iface_;
    BrokerMatchMessageHandler* message_match_handler_;
    string id_;
    size_t num_of_threads_;
    TaskScheduler<std::function<void()>> task_scheduler_;
};

/**
 * \brief Helper class to pass to the forwarding table.
 **/
class BrokerMatchMessageHandler : public siena::MatchMessageHandler {
    public:
        BrokerMatchMessageHandler(Broker* broker) : broker_(broker) {}
        virtual ~BrokerMatchMessageHandler () {}
        virtual bool output (siena::if_t iface, const siena::message& msg) {
            return broker_->handle_match(iface, msg);
        }
    private:
        Broker* broker_;
};

} /* namespace mana */

#endif /* BROKER_H_ */
