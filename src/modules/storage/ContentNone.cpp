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


#include "ContentNone.h"
#include "FSController.h"
#include "defines.h"
#include "utils.h"

#include <list>
#include <limits.h>


using namespace std;


ContentNone::ContentNone(const String& path) :
  Content(CONTENT_NONE_TYPE, path)
{
  _props = Props();
}

ContentNone::~ContentNone()
{}



bool
ContentNone::expandable(long long& max_size) const
{
  max_size = LLONG_MAX;
  return true;
}

bool
ContentNone::shrinkable(long long& min_size) const
{
  min_size = 0;
  return true;
}

void
ContentNone::shrink(const String& path,
		    unsigned long long new_size,
		    const Props& new_props)
{}

void
ContentNone::expand(const String& path,
		    unsigned long long new_size,
		    const Props& new_props)
{}

void
ContentNone::apply_props_before_resize(const String& path,
				       unsigned long long old_size,
				       unsigned long long new_size,
				       const Props& new_props)
{}

void
ContentNone::apply_props_after_resize(const String& path,
				      unsigned long long old_size,
				      unsigned long long new_size,
				      const Props& new_props)
{}

bool
ContentNone::removable() const
{
  return true;
}

void
ContentNone::remove()
{
  // nothing to do :)
}


XMLObject
ContentNone::xml() const
{
  XMLObject xml = this->Content::xml();

  /*
  list<XMLObject> remove_us = xml.children();
  for (list<XMLObject>::iterator iter = remove_us.begin();
       iter != remove_us.end();
       iter++)
    xml.remove_child(*iter);
  */

  return xml;
}





void
create_content_none(const counting_auto_ptr<BD>& bd)
{
  long long size = bd->_props.get("size").get_int();
  if (size > 2 * 1024 * 1024)
    size = 2 * 1024 * 1024;

  vector<String> args;
  String out, err;
  int status;

  args.push_back("if=/dev/zero");
  args.push_back(String("of=") + bd->path());
  args.push_back(String("bs=") + utils::to_string(size));
  args.push_back("count=1");

  if (utils::execute("/bin/dd", args, out, err, status))
    throw command_not_found_error_msg("dd");
  if (status)
    throw String("dd failed: ") + out + " " + err + " " + utils::to_string(status);
}


ContentNoneTemplate::ContentNoneTemplate() :
  ContentTemplate(CONTENT_NONE_TYPE)
{}


ContentNoneTemplate::~ContentNoneTemplate()
{}
