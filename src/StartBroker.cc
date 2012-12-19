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
#include "Broker.h"
#include "Log.h"
//#include "common.h"

using namespace std;

// golbal variable
mana::Broker broker("broker1");

void termination_handler(int signum) {
    broker.shutdown();
}

void start_broker() {
    string url = "tcp:localhost:2350";
    broker.add_transport(url);
    broker.start();
}

int main(void) {
	Log::ReportingLevel() = logINFO;
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
