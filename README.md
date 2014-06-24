# Mana Content-Based Messaging System

Mana is a high throughput content-based, publish/subscribe messaging broker written in C++11.
It also provides a simple C++ API to connect to the broker to subscribe to or publish messages.
At it's heart, Mana uses Siena Fast Forward (SFF) matching engine for matching messages against subscriptions (see http://www.inf.usi.ch/carzaniga/cbn/forwarding/index.html).
Mana supports both UDP and TCP (in persistent as well as on-demand connection mode) and uses Protocol Buffers for fast serialization and ease
of writing clients in other languages.

## Requirements

Mana is developed and tested on Linux with G++ 4.6 and takes advantage of C++11 features.
In order to compile Mana broker and write clients, you need the following software:

- CMake
- Boost libraries (version 4.50 or better)
- libsff (you can obtain the library from here http://www.inf.usi.ch/carzaniga/siena/forwarding/index.html)
- GCC (4.6 or better)

## Compiling the broker and the library

To compile, from the project's root directory simply type
    make
To run the test suite run
    make test

###### Tutorial

In this tutorial we will show to write a simple subscriber, a simple publisher, and how to setup and run the Mana message broker.

Every client, be it a subscriber or a publisher connects to the broker using an instance
of the class Mana::Context. An object of this class will also provide facilities to subscribe
to messages with certain content and publish messages. A context can only be associate to a
single broker. So, a client needing to communicate with multiple brokers has to instantiate multiple instances of this class.

###### 1. Writing  a subscriber ###

Let us start by writing a simple subscriber. Let's first take a look at the source code for
the subscriber class:

    #include <iostream>
    #include <functional>
    #include <memory>
    #include <Mana.h>

    class Subscriber {

        public:

            void handle_notification(const mana::ManaMessage& m) {
                cout << endl << "Application received notification: ";
                for(auto attr : m)
                    std::cout << attr.name().begin << " ";
            }

            void stop() {
                try {
                    context_->stop();
                } catch(exception& e) {
                    cout << endl << e.what();
                    exit(-1);
                }
            }

        void start() {
            try {
                cout << endl << "Starting subscriber client...";
                string rem_url = "ka:127.0.0.1:2350";
                string loc_url = "ka:127.0.0.1:3361";
                auto hndlr = std::bind(&Subscriber::handle_notification, this, std::placeholders::_1);
                context_ = make_shared<mana::ManaContext>("subscriber", loc_url, rem_url, hndlr);
                context_->start();

                // first subscription
                mana::ManaFilter f1;
                f1.add("cpu usage", mana::ops<int>::eq(), 100);
                context_->subscribe(f1);

                //second subscription
                mana::ManaFilter f2;
                f2.add("node_id", mana::ops<const char*>::eq(), "server1.example.com").add("temperature", mana::ops<int>::gt(), 36);
                context_->subscribe(f2);

                //wait for notifications...
                context_->join();

            } catch(exception& e) {
                cout << endl << e.what();
                exit(-1);
            }
        }


        private:

            shared_ptr<mana::ManaContext> context_;
    }; // class Subscriber


    int main() {
        Subscriber subscriber;
        subscriber.start();
        return 0;
    }



Finally, let's compile our program

    g++ -std=c++11 subscriber.cc -o subscriber -I../src/ -I/opt/include/ -L../bin -L/opt/lib -lmana -lprotobuf -lpthread -lsff -lboost_system


###### 2. Writing  a publisher ###

Now, we write our simple publisher.

    #include <iostream>
    #include <functional>
    #include <memory>
    #include <Mana.h>

    class Publisher {

        public:

            void handle_notification(const mana::ManaMessage& m) {
                cout << endl << "Application received notification: ";
                for(auto attr : m)
                    std::cout << attr.name().begin << " ";
            }

            void stop() {
                try {
                    context_->stop();
                } catch(exception& e) {
                    cout << endl << e.what();
                    exit(-1);
                }
            }

        void start() {
            try {

                cout << endl << "Starting publisher client...";
                string rem_url = "ka:127.0.0.1:2350";
                string loc_url = "ka:127.0.0.1:3362";
                auto hndlr = std::bind(&Publisher::handle_notification, this, std::placeholders::_1);
                context_ = make_shared<mana::ManaContext>("publisher", loc_url, rem_url, hndlr);
                context_->start();
                // publish messages
                for(int i = 0; i <= 100; i++) {
                    mana::ManaMessage msg1;
                    msg1.add("cpu usage", i);
                    context_->publish(msg1);
                }

                mana::ManaMessage msg2;
                msg2.add("node_id", "server1.example.com").add("temperature", 40);
                context_->publish(msg2);

                cout << endl << "done.";
                sleep(1);
                context_->stop();
            } catch(mana::ManaException& e) {
                    cout << endl << e.what();
                exit(-1);
            }
        }


        private:

            shared_ptr<mana::ManaContext> context_;
    };


    int main() {
        Publisher publisher;
        publisher.start();
        return 0;
    }


And just like the subscriber, we compile our client as the following

    g++ -std=c++11 publisher.cc -o publisher -I../src/ -I/opt/include/ -L../bin -L/opt/lib -lmana -lprotobuf -lpthread -lsff -lboost_system

###### 3. Configuring and running the broker ###
