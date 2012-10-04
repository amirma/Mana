/*
 * SienaPlusException.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "SienaPlusException.h"
#include <string>
#include <iostream>
#include "common.h"

using namespace std;

namespace sienaplus {

SienaPlusException::SienaPlusException() {
}

SienaPlusException::SienaPlusException(const char* msg) {
	log_err(msg);
}

SienaPlusException::SienaPlusException(const string& msg) {
	log_err(msg);
}

SienaPlusException::~SienaPlusException() throw(){
}

} /* namespace sienaplus */
