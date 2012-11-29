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

#include <boost/asio.hpp>
#include "MessageReceiver.h"
#include "ManaMessage.pb.h"
#include "NetworkConnector.h"
#include <array>
#include <queue>
#include <mutex>
#include <chrono>
#include <memory>
#include <siena/fwdtable.h>
#include "TaskScheduler.h"

#define DEFAULT_NUM_OF_THREADS 5
#define DEFAULT_HEARTBEAT_INTERVAL_SECONDS 5

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
    /** @brief Represents a neighboring node (client, broker) with which a broker has
     * a session.
    */
    struct NeighborNode {
        NeighborNode(NetworkConnector* nc,const string& id, siena::if_t ifc) :
            net_connector_(nc), id_(id), iface_(ifc) {}
        NeighborNode(const NeighborNode&) = delete;
        NeighborNode& operator=(const NeighborNode&) = delete;

        NetworkConnector* net_connector_;
        string id_;
        siena::if_t iface_;
        /** The time at which we last received a heartbeat from this neighbor */
        std::chrono::time_point<std::chrono::system_clock> last_hb_reception_ts_;
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
    void handle_sub(NetworkConnector*, ManaMessage&);
    void handle_not(ManaMessage&);
    void handle_heartbeat(ManaMessage&);
    // we want BrokerMatchMessageHandler to be able to call
    // the private method 'handle_match'
    friend class BrokerMatchMessageHandler;
private:
    // private methods.
    void check_neighbors_and_send_hb();
    // class properties
    boost::asio::io_service io_service_;
    vector<shared_ptr<MessageReceiver> > message_receivers;
    //data, size
    void receive_handler(NetworkConnector*, ManaMessage&);
    void connect_handler(shared_ptr<NetworkConnector>);
    // the main forwarding table
    //
    FwdTableWrapper fwd_table_wrapper_;
    // a number generator for generating unique numbers to represnt
    // clients/neighbors
    IFaceNoGenerator iface_no_generator_;
    // map interface/client/neighbors to the connections
    map<string, shared_ptr<NeighborNode>> neighbors_by_id_;
    map<siena::if_t, shared_ptr<NeighborNode>> neighbors_by_iface_;
    BrokerMatchMessageHandler* message_match_handler_;
    bool handle_match(siena::if_t, const siena::message&);
    string id_;
    size_t num_of_threads_;
    TaskScheduler task_scheduler_;
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
