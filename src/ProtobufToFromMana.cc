/*
 * This file contains functions for conversion between protobuf and siena
 * simepl types.
 */
#include "common.h"
#include "Log.h"
#include "ManaFwdTypes.h"
#include "ManaMessageProtobuf.pb.h"

using namespace std;

namespace mana {

void to_ManaMessage(const ManaMessageProtobuf& buff, ManaMessage& msg) {
    for(int i = 0; i < buff.notification().attribute().size(); i++) {
        const string& st = buff.notification().attribute(i).name();
        switch(buff.notification().attribute(i).value().type()) {
        case ManaMessageProtobuf_tag_type_t_STRING: {
            msg.add(st.c_str(), buff.notification().attribute(i).value().string_value());
            break;
        }
        case ManaMessageProtobuf_tag_type_t_INT: {
            msg.add(st.c_str(), buff.notification().attribute(i).value().int_value());
            break;
        }
        case ManaMessageProtobuf_tag_type_t_DOUBLE: {
            msg.add(st.c_str(), buff.notification().attribute(i).value().double_value());
            break;
        }
        case ManaMessageProtobuf_tag_type_t_BOOL: {
            msg.add(st.c_str(), buff.notification().attribute(i).value().bool_value());
            break;
        }
        default:
        	FILE_LOG(logWARNING) << "ProtobufToFromMana::to_ManaMessage: ignoring unrecognized attribute type";
        }
    }
}

void to_ManaFilter(const ManaMessageProtobuf& buff, ManaFilter& fltr) {
    for(int i = 0; i < buff.subscription().constraints_size(); i++) {
        const string& st = buff.subscription().constraints(i).name();
        siena::string_t name(st.c_str());
        siena::operator_id op_id = siena::operator_id(buff.subscription().constraints(i).op());
        switch(buff.subscription().constraints(i).value().type()) {
        case ManaMessageProtobuf_tag_type_t_STRING: {
            const string& st = buff.subscription().constraints(i).value().string_value();
            mana_op_value* sopv =
                new mana_op_value(op_id, siena::string_t(st.c_str()));
            fltr.add(name, sopv);
            break;
        }
        case ManaMessageProtobuf_tag_type_t_INT: {
            mana_op_value* sopv =
                new mana_op_value(op_id, static_cast<siena::int_t>(buff.subscription().constraints(i).value().int_value()));
            fltr.add(name, sopv);
            break;
        }
        case ManaMessageProtobuf_tag_type_t_DOUBLE: {
            mana_op_value* sopv =
                new mana_op_value(op_id, static_cast<siena::double_t>(buff.subscription().constraints(i).value().double_value()));
            fltr.add(name, sopv);
            break;
        }
        case ManaMessageProtobuf_tag_type_t_BOOL: {
            mana_op_value* sopv  =
                new mana_op_value(op_id, static_cast<siena::bool_t>(buff.subscription().constraints(i).value().bool_value()));
            fltr.add(name, sopv);
            break;
        }
        default:
        	FILE_LOG(logWARNING) << "ProtobufToFromMana::to_ManaFilter: ignoring unrecognized attribute type";
        	break;
        }
    }
}

/*  Fill in the protobuf from a ManaMessage.
 *  NOTE: the required field 'sender' is not set here. It must
 *  be set separately by the caller.
 *  */
void to_protobuf(const ManaMessage& msg, ManaMessageProtobuf& buff) {
    buff.set_type(ManaMessageProtobuf_message_type_t_NOT);
    ManaMessageProtobuf_notification_t* notification = buff.mutable_notification();
    // interate through message attributes and add them to the protobuf
    for(auto& attr : msg) {
        ManaMessageProtobuf_notification_t_attribute_t* att =  notification->add_attribute();
        att->set_name(attr.name().begin);
        att->mutable_value()->set_type(ManaMessageProtobuf_tag_type_t(attr.type()));
        switch(attr.type()) {
            case siena::string_id:
                att->mutable_value()->set_string_value(attr.string_value().begin);
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
            /*
            case siena::any_id:
                log_warn("ProtobugToFromMana::to_protobuf: type \'any_id\' is not supported");
                break;
            */
            default:
            	FILE_LOG(logWARNING) << "ProtobufToFromMana::to_protobuf: ignoring unrecognized attribute type";
        }
    }
}

/*  Fill in the protobuf from a ManaMessage.
 *  NOTE: the required field 'sender' is not set here. It must
 *  be set separately by the caller.
 *  */
void to_protobuf(const ManaFilter& fltr, ManaMessageProtobuf& buff) {
    // set message type
    buff.set_type(ManaMessageProtobuf_message_type_t_SUB);
    ManaMessageProtobuf_subscription_t* s = buff.mutable_subscription();
    for(auto& cnst : fltr) {
    	ManaMessageProtobuf_subscription_t_constraint_t* c = s->add_constraints();
    	c->set_name(cnst.name().begin);
    	c->mutable_value()->set_type(ManaMessageProtobuf_tag_type_t(cnst.type()));
    	c->set_op(ManaMessageProtobuf_subscription_t_operator_t(cnst.op()));
        switch(cnst.type()) {
            case siena::type_id::string_id:
            	c->mutable_value()->set_string_value(cnst.string_value().begin);
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
            /*
            case siena::type_id::anytype_id:
                log_err("Type any is not supported yet.");
                break;
            */
            default:
            	FILE_LOG(logWARNING) << "ProtobufToFromMana::to_protobuf: ignoring unrecognized attribute type";

       }

        //make sure all required fields are filled
        assert(c->has_name());
        assert(c->has_op());
        assert(c->has_value());
    }
}


} // namespace mana
