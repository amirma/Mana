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

using namespace std;

void start_broker() {
	cout << "Starting broker...";
	sienaplus::Broker broker;
	string url = "tcp:localhost:23501";
	broker.add_transport(url);
	broker.start();
}

int main(void) {
	start_broker();
	return 0;
}
