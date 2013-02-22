/*
 * @file StartBroker.cc
 *
 * @brief Parse command line parameters and start a broker
 *
 * @author Amir Malekpour
 * @version 0.1
 *
 * Copyright © 2012 Amir Malekpour
 *
 *  Mana is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mana is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. For more details see the GNU General Public License
 *  at <http: *www.gnu.org/licenses/>
 */

#include <iostream>
#include <signal.h>
#include <memory>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include "Broker.h"
#include "Log.h"
//#include "common.h"

using namespace std;

// global variable
static shared_ptr<mana::Broker> broker;
static boost::program_options::options_description desc;
static const int default_num_threads = 4;
static const string default_log_severity = "info";

void termination_handler(int signum) {
    broker->shutdown();
}

static void setup_signal_hndlr() {
    // signal handling. This is recommended by glibc documentation:
    // If specific signals are to be ignored (because shell wants
    // so), then the handler should remain the same. (ignore).
    if (signal (SIGINT, termination_handler) == SIG_IGN)
     signal (SIGINT, SIG_IGN);
    if (signal (SIGHUP, termination_handler) == SIG_IGN)
     signal (SIGHUP, SIG_IGN);
    if (signal (SIGTERM, termination_handler) == SIG_IGN)
     signal (SIGTERM, SIG_IGN);
}

static void start_broker(const boost::program_options::variables_map& vm) {
    const string id = vm["id"].as<string>();
    const size_t tr = vm["threads"].as<int>();
    broker = make_shared<mana::Broker>(id, tr);
    //
    auto url_list = vm["url"].as<vector<string>>();
    for(auto& url : url_list)
        broker->add_transport(url);
    // set logging level
    Log::ReportingLevel() = Log::FromString(vm["log"].as<string>());
    //
    broker->start();
}

static void print_help() {
    cout << "Usage:" << endl << "StartBroker --id <broker id> --url <url>" << endl;
    cout << desc << "\n";
}

static void setup_opts() {
desc.add_options()
    ("help,h", "print help")
    ("id,i",  boost::program_options::value<string>(), "broker identifier (e.g., broker1)")
    ("url,u", boost::program_options::value<vector<string>>(), "transport URL."
         " You can specify multiple trnaports by repeating \"--url <url>\". The syntax for"
         " a valid url is \"protocol:ip-address:port\" where protocol is one of \"tcp\", \"udp\" or \"ka\""
         " e.g., tcp:127.0.0.1:2350.")
    ("log,l", boost::program_options::value<string>()->default_value(default_log_severity), "logging level (error, warn, info, debug, debug1-4)")
    ("threads,t", boost::program_options::value<int>()->default_value(default_num_threads), "number of threads (default = 4)");
}

static void validate_opts(const boost::program_options::variables_map& vm) {
    if (vm.count("help")) {
        print_help();
        exit(-1);
    }
    if(!vm.count("url")) {
        print_help();
        exit(-1);
    }
    if(!vm.count("id")) {
        print_help();
        exit(-1);
    }
    // validate URL formats
    for(auto& url : vm["url"].as<vector<string>>()) {
        if(URL::is_valid(url) == false) {
           cout << "URL is not valid: " << url << endl;
            exit(-1);
        }
    }
}

int main(int argc, char* argv[]) {
    setup_opts();
    // parse command line options
    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    } catch(exception& e) {
        print_help();
        return -1;
    }
    validate_opts(vm);
    setup_signal_hndlr();
    start_broker(vm);
    return 0;
}
