/*
* @file Utility.cc
* @brief A set of utility and helper functions.
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
*/

#include <boost/algorithm/string.hpp>
#include <vector>
#include "common.h"
#include "Utility.h"
#include "ManaException.h"

namespace mana {

using namespace std;

void string_to_mana_message(const string& str, mana_message& m) {
    //log_info("\n" << str);
    try {
        vector<string> attr_vector;
    	boost::split(attr_vector, str, boost::is_any_of(","));
        for(auto attr_item : attr_vector) {
            // get rid of the white space at the two ends of the string...
            size_t len = attr_item.length();
            if(attr_item[0] == ' ' &&  attr_item[len - 1] == ' ')
                attr_item = attr_item.substr(1, len - 2);
            else if(attr_item[0] == ' ')
                attr_item = attr_item.substr(1, len - 1);
            else if(attr_item[len - 1] == ' ')
                attr_item = attr_item.substr(0, len - 1);
            vector<string> tokens;
        	boost::split(tokens, attr_item, boost::is_any_of(" "));
            siena::string_t cst =  tokens[1].c_str();
            // figure out the type of the attribute
            switch(tokens[0][0]) {
                case 's': {
                    siena::string_t val = tokens[2].c_str();
                    mana_value* sv = new mana_value(val);
                    m.add(cst, sv);
                    break;
                }
                case 'i': {
                    siena::int_t val = static_cast<int>(stoi(tokens[2]));
                    mana_value* sv = new mana_value(val);
                    m.add(cst, sv);
                    break;
                }
                case 'd': {
                    siena::double_t val = static_cast<double>(stod(tokens[2]));
                    mana_value* sv = new mana_value(val);
                    m.add(cst, sv);
                    break;
                }
                case 'b': {
                    siena::bool_t val = tokens[2] == "0" ? false : true;
                    mana_value* sv = new mana_value(val);
                    m.add(cst, sv);
                    break;
                }
                default:
                    log_warn("\nWarning: unrecognized type \'" << tokens[0] << "\'");
                    break;
            }
        }
    } catch(exception& e) {
        throw mana::ManaException("string_to_mana_message(): Invalid string.");
    }
}

void string_to_mana_filter(const string& str, mana_filter& f) {
    try {
        //log_info("\n" << str);
        vector<string> constraint_vector;
    	boost::split(constraint_vector, str, boost::is_any_of(","));
        for(auto constraint_item : constraint_vector) {
            // get rid of the white space at the two ends of the string...
            size_t len = constraint_item.length();
            if(constraint_item[0] == ' ' &&  constraint_item[len - 1] == ' ')
                constraint_item = constraint_item.substr(1, len - 2);
            else if(constraint_item[0] == ' ')
                constraint_item = constraint_item.substr(1, len - 1);
            else if(constraint_item[constraint_item.length() - 1] == ' ')
                constraint_item = constraint_item.substr(0, len - 1);
            vector<string> tokens;
        	boost::split(tokens, constraint_item, boost::is_any_of(" "));
            if(!validate_constraint(tokens)) {
                log_warn("\nSimpleClinet::string_to_mana_filter(): Invalid constraint in: " << str);
                continue;
            }
            siena::string_t cst =  tokens[1].c_str();
            // figure out the type of the constraint
            switch(tokens[0][0]) {
                case 's': {
                    siena::string_t val = tokens[3].c_str();
                    mana_op_value* sov = new mana_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                }
                case 'i': {
                    siena::int_t val = static_cast<int>(stoi(tokens[3]));
                    mana_op_value* sov = new mana_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                }
                case 'd': {
                    siena::double_t val = static_cast<double>(stod(tokens[3]));
                    mana_op_value* sov = new mana_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                }
                case 'b': {
                    siena::bool_t val = tokens[3] == "0" ? false : true;
                    mana_op_value* sov = new mana_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                }
                default:
                    log_warn("\nWarning: unrecognized type \'" << tokens[0] << "\'");
                    break;
            }
        }
    } catch(exception& e) {
        throw mana::ManaException("string_to_filter_message(): Invalid string.");
    }
}

bool validate_constraint(const vector<string>& tokens) {
    //TODO:
    return true;
}

} // namespace mana
