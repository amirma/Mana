/*
 * Broker.h
 *
 *  Created on: Sep 7, 2012
 *      Author: amir
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <boost/asio.hpp>
#include "MessageReceiver.h"
#include "SienaPlusMessage.pb.h"
#include "NetworkConnector.h"
#include <array>
#include <queue>
#include <mutex>

#include "siena/fwdtable.h"

#define DEFAULT_NUM_OF_THREADS 4

using namespace std;

namespace sienaplus {

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

        void return_number(siena::if_t n) {
            lock_guard<std::mutex> lock_(mutex_);
            queue_.push(n);
        }

    private:
        siena::if_t last_;
        queue<siena::if_t> queue_;
        mutex mutex_;
};

/** \brief Represents a neighboring node (client, broker) with which a broker has
 * a session.
*/
struct NeighborNode {
    NeighborNode(NetworkConnector* nc,const string& id, siena::if_t ifc) : 
        net_connector_(nc), id_(id), iface_(ifc) {}
    NetworkConnector* net_connector_;
    string id_;
    siena::if_t iface_;
    private:    
    NeighborNode() {} //disable this
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
	void handle_message(SienaPlusMessage&);
	void handle_sub(NetworkConnector*, SienaPlusMessage&);
	void handle_not(SienaPlusMessage&);
    // we want BrokerMatchMessageHandler to be able to call
    // the private method 'handle_match'
    friend class BrokerMatchMessageHandler;
private:
	boost::asio::io_service io_service_;
	vector<shared_ptr<MessageReceiver> > message_receivers;
	//data, size
	void receive_handler(NetworkConnector*, SienaPlusMessage&);
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

} /* namespace sienaplus */

#endif /* BROKER_H_ */
