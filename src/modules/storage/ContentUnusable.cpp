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


#include "ContentUnusable.h"
#include "defines.h"


using namespace std;


ContentUnusable::ContentUnusable(const String& path) :
  Content(CONTENT_UNUSABLE_TYPE, path)
{
  _props = Props();
}

ContentUnusable::~ContentUnusable()
{}


bool
ContentUnusable::expandable(long long& max_size) const
{
  return false;
}

bool
ContentUnusable::shrinkable(long long& min_size) const
{
  return false;
}

void
ContentUnusable::shrink(const String& path,
		    unsigned long long new_size,
		    const Props& new_props)
{
  throw String("ContentUnusable is not shrinkable");
}

void
ContentUnusable::expand(const String& path,
		    unsigned long long new_size,
		    const Props& new_props)
{
  throw String("ContentUnusable is not expandable");
}

void
ContentUnusable::apply_props_before_resize(const String& path,
				       unsigned long long old_size,
				       unsigned long long new_size,
				       const Props& new_props)
{}

void
ContentUnusable::apply_props_after_resize(const String& path,
				      unsigned long long old_size,
				      unsigned long long new_size,
				      const Props& new_props)
{}

bool
ContentUnusable::removable() const
{
  return false;
}

void
ContentUnusable::remove()
{
  throw String("ContentUnusable is not removable");
}


XMLObject
ContentUnusable::xml() const
{
  XMLObject xml = this->Content::xml();

  return xml;
}
