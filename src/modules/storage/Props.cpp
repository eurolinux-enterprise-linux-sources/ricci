/*
  Copyright Red Hat, Inc. 2005

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to the
  Free Software Foundation, Inc.,  675 Mass Ave, Cambridge,
  MA 02139, USA.
*/
/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */


#include "Props.h"
#include "defines.h"
#include "ValidationError.h"


using namespace std;


Props::Props()
{}

Props::Props(const XMLObject& xml)
{
  if (xml.tag() != PROPERTIES_TAG)
    throw String("not properties");

  for (list<XMLObject>::const_iterator iter = xml.children().begin();
       iter != xml.children().end();
       iter++) {
    try {
      Variable var(*iter);
      _set(var);
    } catch ( ... ) {}
  }
}

Props::~Props()
{}


bool
Props::has(const String& name) const
{
  return _vars.find(name) != _vars.end();
}

const Variable
Props::get(const String& name) const
{
  map<String, Variable>::const_iterator iter = _vars.find(name);
  if (iter == _vars.end())
    throw String("Props: no such variable ") + name;
  return iter->second;
}

void
Props::set(const Variable& var)
{
  // check conditionals
  String c(var.get_conditional_bool_if());
  if (!c.empty())
    if (get(c).type() != Boolean)
      throw String("non-boolean variable used as bool condition for variable ") + var.name();
  c = var.get_conditional_bool_ifnot();
  if (!c.empty())
    if (get(c).type() != Boolean)
      throw String("non-boolean variable used as bool condition for variable ") + var.name();

  // insert new variable
  _set(var);
}

void
Props::_set(const Variable& var)
{
  pair<const String, Variable> p(var.name(), var);
  pair<map<const String, Variable>::iterator, bool> i = _vars.insert(p);
  if (i.second == false)
    i.first->second = var;
}

bool
Props::is_active(const Variable& var) const
{
  String c(var.get_conditional_bool_if());
  if (!c.empty())
    if (!get(c).get_bool())
      return false;
  c = var.get_conditional_bool_ifnot();
  if (!c.empty())
    if (get(c).get_bool() == true)
      return false;
  return true;
}

void
Props::validate() const
{
  validate(*this);
}

void
Props::validate(const Props& props) const
{
  for (map<String, Variable>::const_iterator iter = _vars.begin();
       iter != _vars.end();
       iter++) {
    const Variable& v1 = iter->second;
    if (props.is_active(v1)) {
      const Variable v2(props.get(iter->first));
      if (v1.mutabl()) {
	if (! v1.validate(v2))
	  throw ValidationError();
      } else {
	if (! v1.equal(v2))
	  throw ValidationError();
      }
    }
  }
}


XMLObject
Props::xml() const
{
  XMLObject props(PROPERTIES_TAG);
  for (map<String, Variable>::const_iterator iter = _vars.begin();
       iter != _vars.end();
       iter++)
    props.add_child(iter->second.xml());
  return props;
}
