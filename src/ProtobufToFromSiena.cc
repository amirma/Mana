/*
 * This file contains functions for conversion between protobuf and siena
 * simepl types.
 */
#include "common.h"
#include <iostream>

using namespace std;

namespace sienaplus { 

void to_simple_message(const SienaPlusMessage& buff, simple_message& msg) {
    for(int i = 0; i < buff.notification().attribute().size(); i++) {
        string* st = new string(buff.notification().attribute(i).name());
        siena::string_t name(st->c_str());
        switch(buff.notification().attribute(i).value().type()) {
        case SienaPlusMessage_tag_type_t_STRING: {
            // TODO : I don't like the conversion of string to siena string
            // ...
            string* st = new string(buff.notification().attribute(i).value().string_value());
            // TODO: string_t again...
            simple_value* sv = new simple_value(siena::string_t(st->c_str()));
            msg.add(name, sv);
            break;
        }
        case SienaPlusMessage_tag_type_t_INT: {
            simple_value* sv = 
                new simple_value(static_cast<siena::int_t>(buff.notification().attribute(i).value().int_value()));
            msg.add(name, sv);
            break;
        }
        case SienaPlusMessage_tag_type_t_DOUBLE: {
            simple_value* sv = 
                new simple_value(static_cast<siena::double_t>(buff.notification().attribute(i).value().double_value()));
            msg.add(name, sv);
            break;
        }
        case SienaPlusMessage_tag_type_t_BOOL: {
            simple_value* sv  = 
                new simple_value(static_cast<siena::bool_t>(buff.notification().attribute(i).value().bool_value()));
            msg.add(name, sv);
            break;
        }
        default:
            cout << endl << "Warining: Broker.cc::handle_sub(): ignoring unrecognized attribute type";
        }
    }
}

void to_simple_filter(const SienaPlusMessage& buff, simple_filter& fltr) {
    for(int i = 0; i < buff.subscription().constraints_size(); i++) {
        string* st = new string(buff.subscription().constraints(i).name());
        // TODO: string_t
        siena::string_t name(st->c_str());
        siena::operator_id op_id = siena::operator_id(buff.subscription().constraints(i).op());
        switch(buff.subscription().constraints(i).value().type()) {
        case SienaPlusMessage_tag_type_t_STRING: {
            // TODO : I don't like the conversion of string to siena string
            // ...
            string* st = new string(buff.subscription().constraints(i).value().string_value());
            // TODO: string_t again...
            simple_op_value* sopv = 
                new simple_op_value(op_id, siena::string_t(st->c_str()));
            fltr.add(name, sopv);
            break;
        }
        case SienaPlusMessage_tag_type_t_INT: {
            simple_op_value* sopv = 
                new simple_op_value(op_id, static_cast<siena::int_t>(buff.subscription().constraints(i).value().int_value()));
            fltr.add(name, sopv);
            break;
        }
        case SienaPlusMessage_tag_type_t_DOUBLE: {
            simple_op_value* sopv = 
                new simple_op_value(op_id, static_cast<siena::double_t>(buff.subscription().constraints(i).value().double_value()));
            fltr.add(name, sopv);
            break;
        }
        case SienaPlusMessage_tag_type_t_BOOL: {
            simple_op_value* sopv  = 
                new simple_op_value(op_id, static_cast<siena::bool_t>(buff.subscription().constraints(i).value().bool_value()));
            fltr.add(name, sopv);
            break;
        }
        default:
            cout << endl << "Warining: Broker.cc::handle_sub(): ignoring unrecognized constraint type";
        }
    }
}

/*  Fill in the protobuf from a simple_message.
 *  NOTE: the required field 'sender' is not set here. It must 
 *  be set separately by the caller. 
 *  */
void to_protobuf(const simple_message& msg, SienaPlusMessage& buff) {
    buff.set_type(SienaPlusMessage_message_type_t_NOT);
    SienaPlusMessage_notification_t* notification = buff.mutable_notification();
    // interate through message attributes and add them to the protobuf
    for(auto& attr : msg) {
        SienaPlusMessage_notification_t_attribute_t* att =  notification->add_attribute();
        att->set_name(attr.name().c_str());
        att->mutable_value()->set_type(SienaPlusMessage_tag_type_t(attr.type()));
        switch(attr.type()) {
            case siena::string_id:
                att->mutable_value()->set_string_value(attr.string_value().c_str());
                break;
            case siena::int_id:
                att->mutable_value()->set_int_value(attr.int_value());
                break;
            case siena::double_id:
                att->mutable_value()->set_double_value(attr.double_value());
                break;
            case siena::bool_id:
                att->mutable_value()->set_bool_value(attr.bool_value());
                break;
            case siena::any_id:
                cout << "ProtobugToFromSiena::to_protobuf: type \'any_id\' is not supported";
                break;
        }
    }
}

/*  Fill in the protobuf from a simple_message.
 *  NOTE: the required field 'sender' is not set here. It must 
 *  be set separately by the caller. 
 *  */
void to_protobuf(const simple_filter& fltr, SienaPlusMessage& buff) {
    // set message type
    buff.set_type(SienaPlusMessage_message_type_t_SUB);
    SienaPlusMessage_subscription_t* s = buff.mutable_subscription();
    for(auto& cnst : fltr) {
    	SienaPlusMessage_subscription_t_constraint_t* c = s->add_constraints();
    	c->set_name(cnst.name().c_str());
    	c->mutable_value()->set_type(SienaPlusMessage_tag_type_t(cnst.type()));
    	c->set_op(SienaPlusMessage_subscription_t_operator_t(cnst.op()));
        switch(cnst.type()) {
            case siena::type_id::string_id:
            	c->mutable_value()->set_string_value(cnst.string_value().c_str());
                break;
            case siena::type_id::int_id:
            	c->mutable_value()->set_int_value(cnst.int_value());
                break;
            case siena::type_id::double_id:
            	c->mutable_value()->set_double_value(cnst.double_value());
                break;
            case siena::type_id::bool_id:
            	c->mutable_value()->set_bool_value(cnst.bool_value());
                break;
            case siena::type_id::anytype_id:
            	cout << "Type any is not supported yet.";
                break;
       }

        //make sure all required fields are filled
        assert(c->has_name());
        assert(c->has_op());
        assert(c->has_value());
    }

}


} // namespace sienaplus
