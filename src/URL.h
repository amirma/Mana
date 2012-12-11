/**
* @file URL.h
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
**/

#ifndef URL_H_
#define URL_H_

#include <string>
#include "common.h"

using namespace std;

namespace mana {

/**
 * @brief Represents a URL.
 */
class URL {
public:
    /**
     * @brief Takes a URL string in the form of <b>protocol:ip_address:port</b>
     *
     * protocol is must be a member of {@link mana::connection_type}. If the input string is not valid
     * an instance of {@link ManaException} will be thrown.
     */
    explicit URL(const string& url);
    URL(const URL& url); // copy ctor
    URL& operator=(const URL& url); // copy assignment
    virtual ~URL();
    const string& url() const;
    unsigned int port() const;
    const string& address() const;
    connection_type protocol() const;
private:
    string url_;
    unsigned int port_;
    string ip_;
    string address_;
    connection_type protocol_;
};

} // namespace mana

#endif /* URL_H_ */
