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

SienaPlusException::SienaPlusException(const string& str): message_(str) {
}

SienaPlusException::~SienaPlusException() throw(){}

const char* SienaPlusException::what() {
    return message_.c_str();
}

} /* namespace sienaplus */
