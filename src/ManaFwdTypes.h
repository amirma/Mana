/*
 * @file ManaFwdTypes.h
 * @brief Concerete implementation of Siena types
 *
 * This implementation is based on simple_fwd_types by Antonio Carzaniga
 *
 * @author Amir Malekpour
 * @version 0.1
 *
 * Copyright © 2012 Amir Malekpour
 *
 * Mana is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mana is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. For more details see the GNU General Public License
 * at <http: *www.gnu.org/licenses/>
 */

#ifndef MANA_FWD_TYPES_H
#define MANA_FWD_TYPES_H

#include <list>
#include <map>
#include <vector>
#include <string.h>
#include <assert.h>

#include <siena/types.h>

const int DEFAULT_MEM_ITEM_NUMBER = 10;

class MemoryManager {
    public:
    MemoryManager() : data_(DEFAULT_MEM_ITEM_NUMBER) //default number of entries
    // in the memory manager.
    {}

    ~MemoryManager() {
        for(auto item : data_)
            delete[] item;
    }

    siena::string_t get_managed_copy(siena::string_t str) {
        char* t = new char[str.length() + 1];
        strcpy(t, str.begin);
        assert(t[str.length()] == '\0');
        data_.push_back(t);
        return siena::string_t(t);
    }

    private:
    std::vector<char*> data_;
};

class mana_value: public siena::value {
public:
    virtual siena::type_id	type() const;
    virtual siena::int_t	int_value() const;
    virtual siena::string_t	string_value() const;
    virtual siena::bool_t	bool_value() const;
    virtual siena::double_t	double_value() const;

    mana_value();
    mana_value(siena::int_t x);
    mana_value(siena::bool_t x);
    mana_value(siena::double_t x);
    mana_value(const siena::string_t & x);

    virtual ~mana_value();

private:
    siena::type_id t;
    union {
	siena::int_t i;
	siena::bool_t b;
	siena::double_t d;
    };
    siena::string_t s;
};

typedef std::map<siena::string_t, const mana_value *> attribute_map;

class mana_attribute : public siena::message::iterator {
public:
    virtual siena::string_t	name() const;
    virtual siena::type_id	type() const;
    virtual siena::int_t	int_value() const;
    virtual siena::string_t	string_value() const;
    virtual siena::bool_t	bool_value() const;
    virtual siena::double_t	double_value() const;
    virtual bool		next();

    virtual bool operator!= (const mana_attribute & other) const;
    const mana_attribute& operator++ ();
    mana_attribute& operator* ();

    mana_attribute(attribute_map::const_iterator b,
		     attribute_map::const_iterator e);
private:
    attribute_map::const_iterator i;
    attribute_map::const_iterator end;
};

class mana_message: public siena::message {
public:
    virtual iterator *	first() const;
    virtual mana_attribute    begin() const;
    virtual mana_attribute	end() const;
    virtual iterator *	find(const siena::string_t & name) const;
    virtual bool	contains(const siena::string_t & name) const;
    bool		add(const siena::string_t & name,
			    const mana_value * a);

    mana_message();

    virtual ~mana_message();

private:
    attribute_map attrs;
    MemoryManager memmgr;
};

class mana_op_value: public mana_value {
public:
    virtual siena::operator_id	op() const;

    mana_op_value(siena::operator_id xo);
    mana_op_value(siena::operator_id xo, siena::int_t x);
    mana_op_value(siena::operator_id xo, siena::bool_t x);
    mana_op_value(siena::operator_id xo, siena::double_t x);
    mana_op_value(siena::operator_id xo, const siena::string_t & x);

private:
    siena::operator_id o;
};

typedef std::multimap<siena::string_t, const mana_op_value *> constraint_map;

class mana_constraint: public siena::filter::iterator {
public:
    virtual siena::string_t	name() const;
    virtual siena::type_id	type() const;
    virtual siena::int_t	int_value() const;
    virtual siena::string_t	string_value() const;
    virtual siena::bool_t	bool_value() const;
    virtual siena::double_t	double_value() const;
    virtual siena::operator_id	op() const;
    virtual bool operator!= (const mana_constraint& other) const;
    const mana_constraint& operator++ ();
    virtual bool		next();
    mana_constraint& operator* ();
    mana_constraint(constraint_map::const_iterator b,
		      constraint_map::const_iterator e);
private:
    constraint_map::const_iterator i;
    constraint_map::const_iterator end;
};

class mana_filter: public siena::filter {
public:
    virtual iterator *		first() const;
    void			add(const siena::string_t name,
				    const mana_op_value * v);

    mana_constraint begin() const;
    mana_constraint end() const;

    mana_filter();
    virtual ~mana_filter();

private:
    constraint_map constraints;
    MemoryManager memmgr;
};

typedef std::list<mana_filter *> filter_list;

class mana_predicate_i: public siena::predicate::iterator {
public:
    virtual filter::iterator * first() const;
    virtual bool		next();

    mana_predicate_i(filter_list::const_iterator b,
		       filter_list::const_iterator e);
private:
    filter_list::const_iterator i;
    filter_list::const_iterator end;
};

class mana_predicate : public siena::predicate {
public:
    virtual iterator *		first() const;
    void			add(mana_filter * v);
    mana_filter *		back();

    mana_predicate();
    virtual ~mana_predicate();

private:
    filter_list filters;
};

#include "ManaFwdTypes.icc"

#endif
