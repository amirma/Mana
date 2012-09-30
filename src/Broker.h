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

        unsigned int borrow_number() {
            lock_guard<std::mutex> lock_(mutex_);
            if(queue_.size() == 0)
                return ++last_;
            unsigned int r = queue_.front();
            queue_.pop();
            return r;
        }

        void return_number(unsigned int n) {
            lock_guard<std::mutex> lock_(mutex_);
            queue_.push(n);
        }

    private:
        unsigned int last_;
        queue<unsigned int> queue_;
        mutex mutex_;
};

struct NeighborNode {
    unsigned int interface_;
    shared_ptr<NetworkConnector> net_connector_;
    string id_;

    NeighborNode() : id_("") {}
};

class Broker {
public:
	Broker();
	virtual ~Broker();
	void start();
	void shutdown();
	boost::asio::io_service& io_service();

	// Use this method to add more transport protocols to the broker
	void add_transport(string);
	void handle_message(SienaPlusMessage&);
	void handle_sub(SienaPlusMessage&);
	void handle_not(SienaPlusMessage&);
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
    map<string, NeighborNode> neighbors_;
};

} /* namespace sienaplus */
#endif /* BROKER_H_ */
