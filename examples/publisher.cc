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

