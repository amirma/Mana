/*
 * TestClient.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include <iostream>
#include <functional>
#include "SienaPlusContext.h"
#include "sff.bzr/simple_fwd_types.h"

using namespace std;

void handle_notification(const simple_message& m) {
	cout << endl << "notification: " ;
}

void start_client() {
	cout << "Starting client...";
	string url = "ka:127.0.0.1:2350";
	sienaplus::SienaPlusContext context(url, std::bind(handle_notification, std::placeholders::_1));
    context.set_id("node1");
	context.start();
    siena::int_t v1 = 5;
    simple_op_value* sov1 = new simple_op_value(siena::operator_id::eq_id, v1);
    siena::string_t cst1 = "const1";
    simple_filter f;
    f.add(cst1, sov1);
	context.subscribe(f);
    // publish a message
    for(int i = 0; i < 1000; i++) {
        simple_value* sv1 = new simple_value(static_cast<siena::int_t>(5));
        simple_message msg1;
        msg1.add(cst1, sv1);
        context.publish(msg1);
    }
    // 
	context.join();
}

int main(void) {
	start_client();
	return 0;
}
