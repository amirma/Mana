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
            string loc_url = "ka:127.0.0.1:3352";
            auto hndlr = std::bind(&Subscriber::handle_notification, this, std::placeholders::_1);
            context_ = make_shared<mana::ManaContext>("node-sub", loc_url, rem_url, hndlr);
            context_->start();

            // first subscription
            mana::ManaFilter f1;
            f1.add("const1", mana::ops<int>::eq(), 5);
            context_->subscribe(f1);

            //second subscription
            mana::ManaFilter f2;
            f2.add("const2", mana::ops<const char*>::eq(), "Mr. Bean").add("age", mana::ops<int>::eq(), 30);
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

