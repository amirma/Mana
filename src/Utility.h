/*
* @file Utility.h
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

#ifndef  _UTILITY_H_
#define  _UTILITY_H_

#include <string>

using namespace std;

namespace mana {

class ManaMessage;
class ManaFilter;


/*
 *  @brief Take a string and fill in the fields of the message
 */
void string_to_ManaMessage(const string& str, ManaMessage& m);

/*
 *  @brief Take a string and fill in the fields of the filter
 */
void string_to_ManaFilter(const string& str, ManaFilter& f);

/*
 * \brief receive a tuple of tokens in the form of <type,name,operator,value>
 * and verify it's semantic correctness.
 *
 * By semantic correctness we mean a logical match between 'type' and
 * 'operator'. For instance with a type siena::bool_t we can only have
 * 'eq_id'.
 */
bool validate_constraint(const vector<string>& tokens);

} //namespace mana

#endif //  _UTILITY_H_
