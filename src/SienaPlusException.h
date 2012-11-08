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

// virtual inheritence for exceptions is a good practice...
class SienaPlusException: virtual public std::exception {
public:
	SienaPlusException(const string& str = "");
    virtual const char* what();
	virtual ~SienaPlusException() throw();
private:
    string message_;
};

} /* namespace sienaplus */
#endif /* SIENAPLUSEXCEPTION_H_ */
