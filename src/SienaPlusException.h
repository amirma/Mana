/*
 * SienaPlusException.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef SIENAPLUSEXCEPTION_H_
#define SIENAPLUSEXCEPTION_H_

#include <exception>
#include <string>

using namespace std;

namespace sienaplus {

class SienaPlusException: virtual public std::exception {
public:
	SienaPlusException();
	SienaPlusException(const string&);
	SienaPlusException(const char*);
	virtual ~SienaPlusException() throw();
};

} /* namespace sienaplus */
#endif /* SIENAPLUSEXCEPTION_H_ */
