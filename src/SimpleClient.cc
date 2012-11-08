/*
 * TestClient.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: amir
 */
#include "SimpleClient.h"
#include "SienaPlusException.h"

using namespace std;

SimpleClient::SimpleClient(const string& str) : client_id_(str), 
    broker_url_("ka:127.0.0.1:2350"), flag_session_established_(false) {}

SimpleClient::~SimpleClient() {}

void SimpleClient::handle_notification(const simple_message& m) {
    log_info("\nApplication received notification: ");
    for(auto attr : m)
        log_info(attr.name().c_str() << " ");
}

void SimpleClient::start() {
    try {
        log_info("\nStarting client...");
        const string& url =  broker_url_;
        auto hndlr = std::bind(&SimpleClient::handle_notification, this, std::placeholders::_1);
        context_ = make_shared<sienaplus::SienaPlusContext>(client_id_, url, hndlr);
        context_->start();
        run();
        context_->join();
    } catch(sienaplus::SienaPlusException& e) {
        log_err("\n" << e.what() << "\n");
        exit(-1);
    }
}

void SimpleClient::stop() {
    context_->stop();
}

/*  
 *  \brief A concrete implementation could implement this to do something
 *  useful, like subscribing or publishing. See details.
 *
 *  For instance we could do something like this in run()

    // subscribe to something:
    siena::int_t val = 5;
    simple_op_value* sov = new simple_op_value(siena::eq_id, val);
    siena::string_t cst = "const1";
    simple_filter fltr;
    fltr.add(cst, sov);
    context_->subscribe(fltr);

    //or publish something:
    simple_value* sv = new simple_value(static_cast<siena::int_t>(10));
    siena::string_t cst = "const1";
    simple_message msg;
    msg.add(cst, sv);
    context_->publish(msg);
 *
 */
void SimpleClient::run() {}

bool SimpleClient::set_broker(const string& str) {
    if(flag_session_established_) {
        log_err("\nCan not change broker after session is established");
        return false;
    }
    broker_url_ = str;
    return true;
}

simple_message SimpleClient::string_to_simple_message(const string& str) const {
    //log_info("\n" << str);
    try {
        vector<string> attr_vector;
    	boost::split(attr_vector, str, boost::is_any_of(","));
        simple_message m;
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
                    simple_value* sv = new simple_value(val);
                    m.add(cst, sv);
                    break;
                }
                case 'i': {
                    siena::int_t val = static_cast<int>(stoi(tokens[2]));
                    simple_value* sv = new simple_value(val);
                    m.add(cst, sv);
                    break;
                }
                case 'd': {
                    siena::double_t val = static_cast<double>(stod(tokens[2]));
                    simple_value* sv = new simple_value(val);
                    m.add(cst, sv);
                    break;
                } 
                case 'b': {
                    siena::bool_t val = tokens[2] == "1" ? true : false;
                    simple_value* sv = new simple_value(val);
                    m.add(cst, sv);
                    break;
                }
                default:
                    log_warn("\nWarning: unrecognized type \'" << tokens[0] << "\'");
            }
        }
        return m;
    } catch(exception& e) {
        throw sienaplus::SienaPlusException("string_to_simple_message(): Invalid string.");
    }
}

simple_filter SimpleClient::string_to_simple_filter(const string& str) const {
    try {
        //log_info("\n" << str);
        vector<string> constraint_vector;
    	boost::split(constraint_vector, str, boost::is_any_of(","));
        simple_filter f;
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
                log_warn("\nSimpleClinet::string_to_simple_filter(): Invalid constraint in: " << str);
                continue;
            }
            siena::string_t cst =  tokens[1].c_str();
            // figure out the type of the constraint
            switch(tokens[0][0]) {
                case 's': {
                    siena::string_t val = tokens[3].c_str();
                    simple_op_value* sov = new simple_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                }
                case 'i': {
                    siena::int_t val = static_cast<int>(stoi(tokens[3]));
                    simple_op_value* sov = new simple_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                }
                case 'd': {
                    siena::double_t val = static_cast<double>(stod(tokens[3]));
                    simple_op_value* sov = new simple_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                } 
                case 'b': {
                    siena::bool_t val = tokens[3] == "1" ? true : false;
                    simple_op_value* sov = new simple_op_value(siena::eq_id, val);
                    f.add(cst, sov);
                    break;
                }
                default:
                    log_warn("\nWarning: unrecognized type \'" << tokens[0] << "\'");
            }
        }
        return f;
    } catch(exception& e) {
        throw sienaplus::SienaPlusException("string_to_filter_message(): Invalid string.");
    }
}

/*
 * \brief receive a tuple of tokens in the form of <type,name,operator,value>
 * and verify it's semantic correctness.
 *
 * By semantic correctness we mean a logical match between 'type' and
 * 'operator'. For instance with a type siena::bool_t we can only have 
 * 'eq_id'.
 */
bool SimpleClient::validate_constraint(const vector<string>& tokens) const {
    //TODO:
    return true;
}
