/*
 * TestClient.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include <iostream>
#include <fstream>
#include <functional>
#include <memory>
#include <signal.h>
#include <boost/algorithm/string.hpp>
#include "SimpleClient.h"
#include "common.h"
#include "Utility.h"
#include "Log.h"

using namespace std;

class TestClient : public SimpleClient {
public:

    TestClient(const string& str, const string& url, const string& broker) : SimpleClient(str, url, broker) {}

    void run() {
    	ifstream ifs(workload_file.c_str());
        while(ifs.good()) {
            string line;
            getline(ifs, line);
            if(line[0] == '#') //ignore commented lines
                continue;
            vector<string> tokens;
            boost::split(tokens, line, boost::is_any_of(" "));
            if(tokens[0] != client_id_)
                continue;
            size_t first = line.find_first_of("\"");
            size_t second = line.find_first_of("\"", first+1);
            string str = line.substr(first + 1, second - first - 1);
            if(tokens[3] == "pub") {
            	FILE_LOG(logINFO) << "Publishing: " << line;
                //sleep(1);
                context_->publish(str);
            } else if(tokens[3] == "sub") {
            	FILE_LOG(logINFO) << "Subscribing: " << line;
                context_->subscribe(str);
            }
        }
    }

    void set_workload(string& f) {
    	workload_file = f;
    }

    private:
    	string workload_file;
}; // class

shared_ptr<TestClient> client = nullptr;

void termination_handler(int signum) {
	FILE_LOG(logINFO) << "TestClient: Received signal " << signum << ". Client is terminating.";
    client->stop();
    exit(0);
}

void print_usage() {
    cout << endl << "Usage:\n"
    "TestClient -id <id> -url <client url> -broker <broker url> -wkld <workload file>\n";
}

void setup_signal_hndlr() {
    if (signal (SIGINT, termination_handler) == SIG_IGN)
        signal (SIGINT, SIG_IGN);
    if (signal (SIGHUP, termination_handler) == SIG_IGN)
        signal (SIGHUP, SIG_IGN);
    if (signal (SIGTERM, termination_handler) == SIG_IGN)
        signal (SIGTERM, SIG_IGN);
}

int main(int argc, char* argv[]) {
    setup_signal_hndlr();
    string id = "";
    string fname = "";
    string url = "udp:127.0.0.1:3350";
    string broker = "udp:127.0.0.1:2350";
    string log = "info";
    int i = 0;
    while(++i < argc) {
        if(strcmp(argv[i], "-id") == 0 && i + 1 < argc)
            id = string(argv[++i]);
        else if(strcmp(argv[i], "-wkld") == 0 && i + 1 < argc)
        	fname = string(argv[++i]);
        else if(strcmp(argv[i], "-url") == 0 && i + 1 < argc)
            url = string(argv[++i]);
        else if(strcmp(argv[i], "-broker") == 0 && i + 1 < argc)
            broker = string(argv[++i]);
        else if(strcmp(argv[i], "-log") == 0 && i + 1 < argc)
            log = string(argv[++i]);
        else
            goto error;
    }
    if(id == "" || fname == "")
        goto error;
    Log::ReportingLevel() = Log::FromString(log);
    client = make_shared<TestClient>(id, url, broker);
    client->set_workload(fname);
    client->start();
    return 0;
error:
        print_usage();
        exit(-1);
}
