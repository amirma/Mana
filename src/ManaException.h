/*
 * ManaException.h
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */

#ifndef SIENAPLUSEXCEPTION_H_
#define SIENAPLUSEXCEPTION_H_

#include <exception>
#include <string>

using namespace std;

namespace mana {

// virtual inheritence for exceptions is a good practice...
class ManaException: virtual public std::exception {
public:
	ManaException(const string& str = "");
    virtual const char* what();
	virtual ~ManaException() throw();
private:
    string message_;
};

} /* namespace mana */
#endif /* SIENAPLUSEXCEPTION_H_ */
