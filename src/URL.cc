/**
* @file URL.cc
* Interface for MessageSender
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

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "URL.h"
#include "ManaException.h"

namespace mana {

URL::URL(const string& str_url) : url_(str_url) {
	vector<string> tokens;
	boost::split(tokens, url_, boost::is_any_of(":"));
        if(tokens.size() < 3)
	    throw ManaException("Malformed URL or method not supported: " + url_);
	// convert to lowercase e.g., TCP -> tcp
	std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(), ::tolower);
	address_ = tokens[1];
	try {
		port_ = stoi(tokens[2]);
	} catch(const exception& e) {
		throw ManaException("Invalid port number in " + url_);
	}
	if(tokens[0] == "tcp")
		protocol_ = mana::connection_type::tcp;
	else if(tokens[0] == "ka")
		protocol_ = mana::connection_type::ka;
	else if(tokens[0] == "udp")
		protocol_ = mana::connection_type::udp;
	else
		throw ManaException("Malformed URL or method not supported: " + url_);
}

URL::URL(const URL& url) {
	url_ = url.url_;
    protocol_ = url.protocol();
    address_ = url.address();
    port_ = url.port();
}

URL& URL::operator=(const URL& url) {
	url_ = url.url_;
    protocol_ = url.protocol();
    address_ = url.address();
    port_ = url.port();
    return *this;
}

URL::~URL() {}

unsigned int URL::port() const {
	return port_;
}

const string& URL::address() const {
	return address_;
}

connection_type URL::protocol() const {
	return protocol_;
}

const string& URL::url() const {
	return url_;
}

bool URL::is_valid(const string& str) {
	try {
		URL url(str);
	} catch (const exception& e){
		return false;
	}
	return true;
}

} // namespace mana
