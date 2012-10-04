/*
 ============================================================================
 Name        : SienaPlus.cpp
 Author      : Amir Malekpour
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C++,
 ============================================================================
 */

#include <iostream>
#include "Broker.h"
#include <signal.h>
#include "common.h"

using namespace std;


// golbal variable 
sienaplus::Broker broker("broker1");


void termination_handler(int signum) {
    broker.shutdown();
}

void start_broker() {
	log_info("Starting broker...");
	string url = "tcp:localhost:2350";
	broker.add_transport(url);
	broker.start();
}

int main(void) {
    // signal handling. This is recommended by glibc documentation:
    // If specific signals are to be ignored (because shell wants
    // so), then the handler should remain the same. (ignore).
   if (signal (SIGINT, termination_handler) == SIG_IGN)
     signal (SIGINT, SIG_IGN);
   if (signal (SIGHUP, termination_handler) == SIG_IGN)
     signal (SIGHUP, SIG_IGN);
   if (signal (SIGTERM, termination_handler) == SIG_IGN)
     signal (SIGTERM, SIG_IGN);
    // now start a broker
	start_broker();
	return 0;
}
