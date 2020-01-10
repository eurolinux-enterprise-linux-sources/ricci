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


#ifndef MapperFactory_h
#define MapperFactory_h

#include "Mapper.h"
#include <list>


class MapperFactory
{
 public:

  static std::list<std::pair<String, String> >
    get_mapper_ids(const String& mapper_type="");

  static counting_auto_ptr<Mapper>
    get_mapper(const String& mapper_type,
	       const String& mapper_id);

  static std::list<counting_auto_ptr<Mapper> >
    get_mappers(const String& mapper_type="",
		const String& mapper_id="");

  static std::list<counting_auto_ptr<MapperTemplate> >
    get_mapper_templates(const String& mapper_type);

  static counting_auto_ptr<Mapper>
    create_mapper(const MapperTemplate& mapper_temp);

  static counting_auto_ptr<Mapper>
    modify_mapper(const MapperParsed& mapper_temp);

  static void remove_mapper(const MapperParsed& mapper);

  static counting_auto_ptr<Mapper>
    add_sources(const String& mapper_type,
		const String& mapper_id,
		const String& mapper_state_ind,
		const std::list<BDParsed>& bds);

};


#endif  // MapperFactory_h
