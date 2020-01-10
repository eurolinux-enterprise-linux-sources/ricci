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


#ifndef Props_h
#define Props_h

#include "Variable.h"
#include "XML.h"
#include <map>
#include "String.h"


#define PROPERTIES_TAG      String("properties")


class Props
{
 public:
  Props();
  Props(const XMLObject&);
  virtual ~Props();

  void set(const Variable& var);
  bool has(const String& name) const;
  const Variable get(const String& name) const;

  bool is_active(const Variable& var) const;

  void validate() const;
  void validate(const Props& props) const;  // validate props against *this


  XMLObject xml() const;


 private:

  std::map<String, Variable> _vars;

  void _set(const Variable& var);

};


#endif  // Props_h
