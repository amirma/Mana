/*
 * SienaPlusException.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "SienaPlusException.h"
#include <string>
#include <iostream>

using namespace std;

namespace sienaplus {

SienaPlusException::SienaPlusException() {
}

SienaPlusException::SienaPlusException(const char* msg) {
	cout << msg;
}

SienaPlusException::SienaPlusException(const string& msg) {
	cout << msg;
}

SienaPlusException::~SienaPlusException() throw(){
}

} /* namespace sienaplus */
