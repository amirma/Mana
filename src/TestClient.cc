/*
 * TestClient.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include <iostream>
#include "common.h"
#include <functional>
#include <memory>
#include <signal.h>
#include "SienaPlusContext.h"
#include "sff.bzr/simple_fwd_types.h"

using namespace std;

class SimpleClient {
    struct publisher_ {};
    struct subscriber_ {};

    public:

    void handle_notification(const simple_message& m) {
        log_info("\nApplication received notification: ");
        for(auto attr : m)
            log_info(attr.name().c_str() << " ");
    }

    void stop() {
        context_->stop();
    }

    void start(SimpleClient::subscriber_) {
        log_info("\nStarting client...");
        string url = "ka:127.0.0.1:2350";
        auto hndlr = std::bind(&SimpleClient::handle_notification, this, std::placeholders::_1);
        context_ = make_shared<sienaplus::SienaPlusContext>("node-sub", url, hndlr);
        context_->start();
        // fisrt subscription
        siena::int_t v1 = 5;
        simple_op_value* sov1 = new simple_op_value(siena::eq_id, v1);
        siena::string_t cst1 = "const1";
        simple_filter f1;
        f1.add(cst1, sov1);
        context_->subscribe(f1);
        //second subscription
        simple_op_value* sov2 = new simple_op_value(siena::eq_id, "salam");
        siena::string_t cst2 = "const2";
        simple_filter f2;
        f2.add(cst2, sov2);
        context_->subscribe(f2);
        //wait for notifications...
        context_->join();
    }

    void start(SimpleClient::publisher_) {
        log_info("\nStarting client...");
        string url = "ka:127.0.0.1:2350";
        auto hndlr = std::bind(&SimpleClient::handle_notification, this, std::placeholders::_1);
        context_ = make_shared<sienaplus::SienaPlusContext>("node1-pub", url, hndlr);
        context_->start();
        // publish messages
        siena::string_t cst1 = "const1";
        for(int i = 0; i < 10000000; i++) {
            simple_value* sv1 = new simple_value(static_cast<siena::int_t>(5));
            simple_message msg1;
            msg1.add(cst1, sv1);
            context_->publish(msg1);
        }
        siena::string_t cst2 = "const2";
        simple_value* sv2 = new simple_value("salam");
        simple_message msg2;
        msg2.add(cst2, sv2);
        context_->publish(msg2);
        //mail.yahoo.com
        log_info("done.");
        context_->join();
    }

    static publisher_ publisher;
    static subscriber_ subscriber;

    private:
        shared_ptr<sienaplus::SienaPlusContext> context_;
}; // class 


SimpleClient client;

void termination_handler(int signum) {
    log_info("\nTestClient: Received signal " << signum << ". Client is terminating.");
    client.stop();
    exit(0);
}

void print_usage() {
    cout << endl << "Usage:\n"
    "TestClient <-subscriber|-publisher>\n";
}

int main(int argc, char* argv[]) {
    if (signal (SIGINT, termination_handler) == SIG_IGN)
        signal (SIGINT, SIG_IGN);
    if (signal (SIGHUP, termination_handler) == SIG_IGN)
        signal (SIGHUP, SIG_IGN);
    if (signal (SIGTERM, termination_handler) == SIG_IGN)
        signal (SIGTERM, SIG_IGN);
    if(argc < 2) {
        print_usage();
        return -1;
    }
    if(strcmp(argv[1], "-publisher") == 0)
        client.start(SimpleClient::publisher);
    else if(strcmp(argv[1], "-subscriber") == 0)
        client.start(SimpleClient::subscriber);
    else {
        cout << endl << "invalid argument";
        return -1;
    }
	return 0;
}
