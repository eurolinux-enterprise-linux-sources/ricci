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


#include "MapperSource.h"
#include "Mapper.h"


using namespace std;


MapperSource::MapperSource(const String& mapper_type,
			   const String& mapper_id,
			   const String& type,
			   const String& path) :
  Content(CONTENT_MS_TYPE, path),
  _mapper_type(mapper_type),
  _mapper_id(mapper_id),
  _type(type)
{}

MapperSource::~MapperSource()
{}


bool
MapperSource::expandable(long long& max_size) const
{
  return false;
}

bool
MapperSource::shrinkable(long long& min_size) const
{
  return false;
}

void
MapperSource::shrink(const String& path,
		      unsigned long long new_size,
		      const Props& new_props)
{}

void
MapperSource::expand(const String& path,
		      unsigned long long new_size,
		      const Props& new_props)
{}

void
MapperSource::apply_props_before_resize(const String& path,
					unsigned long long old_size,
					unsigned long long new_size,
					const Props& new_props)
{}

void
MapperSource::apply_props_after_resize(const String& path,
				       unsigned long long old_size,
				       unsigned long long new_size,
				       const Props& new_props)
{}


bool
MapperSource::removable() const
{
  return false;
}

void
MapperSource::remove()
{
  throw String("MapperSource not removable");
}


XMLObject
MapperSource::xml() const
{
  XMLObject xml = this->Content::xml();

  //  xml.set_attr("source_type", "source_type");
  xml.set_attr("mapper_type", _mapper_type);
  xml.set_attr("mapper_id", _mapper_id);

  return xml;
}



MapperSourceTemplate::MapperSourceTemplate(const String& mapper_type) :
  ContentTemplate(CONTENT_MS_TYPE),
  _mapper_type(mapper_type)
{
  attrs["mapper_type"] = _mapper_type;
}

MapperSourceTemplate::~MapperSourceTemplate()
{}
