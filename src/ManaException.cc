/*
 * ManaException.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#include "ManaException.h"
#include <string>
#include <iostream>
#include "common.h"

using namespace std;

namespace mana {

ManaException::ManaException(const string& str): message_(str) {
}

ManaException::~ManaException() throw(){}

const char* ManaException::what() {
    return message_.c_str();
}

} /* namespace mana */
