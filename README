# Mana Content-Based Messaging System

Mana is a high throughput content-based, publish/subscribe messaging broker written in C++11.
At it's heart, Mana uses Siena Fast Forward (SFF) matching engine for matching messages
against subscriptions (see http://www.inf.usi.ch/carzaniga/cbn/forwarding/index.html). It also provides
a simple C++ API to connect to the broker.

This is a step by step tutorial to write a simple subscriber and a simple publisher. We will
also show how to configure and start the broker that our clients will connect to.

## Requirements

Mana is developed and tested on Linux with G++ 4.6 and takes advantage of C++11 features.
In order to compile Mana broker and write clients, you need the following software:

- CMake
- Boost libraries (version 4.50 or above)
- libsff (you can obtain the library from here http://www.inf.usi.ch/carzaniga/siena/forwarding/index.html)
- GCC (4.6 or above)

## Compiling the broker and the library

Mana is a Broker software as well as an API to interact with the broker and
connect to the pub/sub network. To compile


###### Tutorial


Every client, be it a subscriber or a publisher connects to the broker using an instance
of the class Mana::Context. An object of this class will also provide facilities to subscribe
to messages with certain content and publish messages. A context can only be associate to a
single broker at a time. So, a client needing to communicate with multiple brokers has to
instantiate multiple instances of this class.

###### 1. Writing  a subscriber ###

Let us start by writing a simple subscriber. Let's first take a look a the source code for
the class:

    #include <iostream>
    #include <functional>
    #include <memory>
    #include "Mana.h"

    class Subscriber {

        public:

        void handle_notification(const mana_message& m) {
            FILE_LOG(logINFO) << "Application received notification: ";
            for(auto attr : m)
                std::cout << attr.name().begin << " ";
        }

        void stop() {
            try {
                context_->stop();
            } catch(exception& e) {/* handle the exception */}
        }

        void start(subscriber_t) {
            try {
                FILE_LOG(logINFO) << "Starting subscriber client...";
                string rem_url = "ka:127.0.0.1:2350";
                string loc_url = "ka:127.0.0.1:3352";
                auto hndlr = std::bind(&SimpleClientInput::handle_notification, this, std::placeholders::_1);
                context_ = make_shared<mana::ManaContext>("node-sub", loc_url, rem_url, hndlr);
                context_->start();
                // first subscription

                context_->subscribe(str);
                //wait for notifications...
                context_->join();
            } catch(exception& e) {
                    std::cout << e.what();
                exit(-1);
            }
        }

        private:

        shared_ptr<mana::ManaContext> context_;
    }; // class Subscriber


    int main(int argc, char* argv[]) {
        SimpleClientInput client;
        client.start(SimpleClientInput::subscriber_t());
        return 0;
    }
Now, let's compile our program

    g++ -std=c++11 subscriber.cc -o subscriber -I../src/ -I/opt/include/ -L../bin -L/opt/lib -lmana -lprotobuf -lpthread -lsff -lboost_system



###### 2. Writing  a publisher ###

Let us start by writing a simple subscriber.


###### 3. Configuring and running the broker ###
