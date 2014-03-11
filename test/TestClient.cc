/*
 * TestClient.cpp
 *
 * Author: Amir Malekpour
 */

#include <iostream>
#include <fstream>
#include <functional>
#include <memory>
#include <signal.h>
#include <boost/algorithm/string.hpp>
#include "Mana.h"

using namespace std;

class TestClient: public SimpleClient {
public:

	TestClient(const string& str, const string& url, const string& broker) :
	    SimpleClient(str, url, broker) {
	}

        void run() {
            try {
                while (cin.good()) {
                    string line;
                    getline(cin, line);
                    if (line[0] == '#' || line.empty()) //ignore commented lines
                        continue;
                    vector<string> tokens;
                    boost::split(tokens, line, boost::is_any_of(" "));
                    if (tokens[0] != client_id_)
                        continue;
                    auto first = line.find_first_of("\"");
                    auto second = line.find_first_of("\"", first + 1);
                    auto str = line.substr(first + 1, second - first - 1);
                    if (tokens[3] == "pub") {
                        cout << endl << "Publishing: " << line;
                        context_->publish(str);
                    } else if (tokens[3] == "sub") {
                        cout << endl << "Subscribing: " << line;
                        context_->subscribe(str);
                    }
                }
            } catch (const exception& e) {
                cout << "Error: " << e.what();
                exit(-1);
            }
        }

}; // class

shared_ptr<TestClient> client = nullptr;

void termination_handler(int signum) {
	cout << endl << "TestClient: Received signal " << signum
			<< ". Client is terminating.";
	client->stop();
	exit(0);
}

void print_usage() {
	cout << endl << "Usage:\n"
			"TestClient -id <id> -url <client url> -broker <broker url>\n"
			"Note that the input workload is read from stdin.\n";
}

void setup_signal_hndlr() {
	if (signal(SIGINT, termination_handler) == SIG_IGN )
		signal(SIGINT, SIG_IGN );
	if (signal(SIGHUP, termination_handler) == SIG_IGN )
		signal(SIGHUP, SIG_IGN );
	if (signal(SIGTERM, termination_handler) == SIG_IGN )
		signal(SIGTERM, SIG_IGN );
}

int main(int argc, char* argv[]) {
	setup_signal_hndlr();
	string id = "";
	string fname = "-";
	string url = "udp:127.0.0.1:3350";
	string broker = "udp:127.0.0.1:2350";
	int i = 0;
	while (++i < argc) {
		if (strcmp(argv[i], "-id") == 0 && i + 1 < argc)
			id = string(argv[++i]);
		else if (strcmp(argv[i], "-url") == 0 && i + 1 < argc)
			url = string(argv[++i]);
		else if (strcmp(argv[i], "-broker") == 0 && i + 1 < argc)
			broker = string(argv[++i]);
		else
			goto error;
	}
	if (id == "" || fname == "")
		goto error;
	client = make_shared<TestClient> (id, url, broker);
	try {
		client->start();
	} catch (const exception& e) {
		cout << "Error: " << e.what();
		exit(-1);
	}
	return 0;
	error: print_usage();
	exit(-1);
}
