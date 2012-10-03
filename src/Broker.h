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

using namespace std;

namespace sienaplus {

/*  this class generates interface numbers and maintains 
*  a list of interface numbers that are freed. */
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

struct NeighborNode {
    siena::if_t interface_;
    shared_ptr<NetworkConnector> net_connector_;
    string id_;
    siena::if_t iface_;
    NeighborNode(const string& id, siena::if_t ifc) : id_(id), iface_(ifc) {}
//    private:
 //   NeighborNode() {} //disable this
};

// forward declaration so that we can have a pointer to 
// the broker in BrokerMatchMessageHandler
class BrokerMatchMessageHandler;

class Broker {
public:
	Broker(const string& id);
	virtual ~Broker();
	void start();
	void shutdown();
	boost::asio::io_service& io_service();

	// Use this method to add more transport protocols to the broker
	void add_transport(string);
	void handle_message(SienaPlusMessage&);
	void handle_sub(SienaPlusMessage&);
	void handle_not(SienaPlusMessage&);
    // we want BrokerMatchMessageHandler to be able to call
    // the private method 'handle_match'
    friend class BrokerMatchMessageHandler;
private:
	boost::asio::io_service io_service_;
	vector<shared_ptr<MessageReceiver> > message_receivers;
	//data, size
	void receive_handler(const char*, int);
    void connect_handler(shared_ptr<NetworkConnector>);
    // the main forwarding table
    siena::FwdTable fwd_table_;
    // a number generator for generating unique numbers to represnt
    // clients/neighbors 
    IFaceNoGenerator iface_no_generator_;
    // map interface/client/neighbors to the connections
    map<string, shared_ptr<NeighborNode>> neighbors_by_id_;
    map<siena::if_t, shared_ptr<NeighborNode>> neighbors_by_iface_;
    BrokerMatchMessageHandler* message_match_handler_;
    bool handle_match(siena::if_t, const siena::message&);
    string id_;
};


class BrokerMatchMessageHandler : public siena::MatchMessageHandler {
    public:
        BrokerMatchMessageHandler(Broker* broker) : broker_(broker) {}
        virtual ~BrokerMatchMessageHandler () {}
        virtual bool output (siena::if_t iface, const siena::message& msg) {
            broker_->handle_match(iface, msg);
        }

    private:
        Broker* broker_;
};


} /* namespace sienaplus */

#endif /* BROKER_H_ */
